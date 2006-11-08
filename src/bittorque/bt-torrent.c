#include <string.h>
#include <stdlib.h>

#include <gnet.h>

#include "bt-manager.h"
#include "bt-peer.h"
#include "bt-torrent.h"
#include "bt-bencode.h"
#include "bt-utils.h"

G_DEFINE_TYPE (BtTorrent, bt_torrent, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_NAME,
	PROP_SIZE
};

/**
 * bt_torrent_parse_file:
 * 
 * Parses the given file as a ".torrent" file, reading all information and setting the torrent's
 * properties. These are bencoded dictionaries, with semantics as defined in the BitTorrent protocol.
 */

static gboolean
bt_torrent_parse_file (BtTorrent *self, gchar *filename, GError **error)
{
	BtBencode *metainfo, *info, *announce, *announce_list, *name, *length, *files, *pieces, *piece_length;
	gchar *contents;

	if (!g_file_get_contents (filename, &contents, NULL, error))
		return FALSE;

	/* decode the bencoded file into our own structure */
	metainfo = bt_bencode_decode (contents, error);

	g_free (contents);

	if (!metainfo)
		return FALSE;

	/* make sure we have the "info" dict */
	info = bt_bencode_lookup (metainfo, "info");
	if (!info || info->type != BT_BENCODE_TYPE_DICT)
		goto cleanup;

	/* check for the "name" entry */
	name = bt_bencode_lookup (info, "name");
	if (!name || name->type != BT_BENCODE_TYPE_STRING)
		goto cleanup;

	self->name = g_strdup (name->value.string->str);
	g_debug ("torrent name: %s", self->name);

	/* check the info hash of this torrent */
	bt_infohash (info, self->infohash);
	bt_hash_to_string (self->infohash, self->infohash_string);
	g_debug ("torrent info hash: %s", self->infohash_string);

	/* find out how to announce to the tracker(s) */
	/* TODO: PROTOCOL: should we be more lenient about missing announce if there is an announce-list? */
	announce = bt_bencode_lookup (metainfo, "announce");
	announce_list = bt_bencode_lookup (metainfo, "announce-list");

	if (announce == NULL)
		goto cleanup;

	self->announce = g_strdup (announce->value.string->str);

	if (announce_list) {
		/* the announce-list is a list of lists of strings (doubly nested) */
		GSList *i;

		self->announce_list = NULL;

		if (announce_list->type != BT_BENCODE_TYPE_LIST)
			goto cleanup;

		for (i = announce_list->value.list; i != NULL; i = i->next) {
			GSList *list, *k;
			BtBencode *j;

			list = NULL;

			j = bt_bencode_slitem (i);
			if (j->type != BT_BENCODE_TYPE_LIST)
				goto cleanup;

			for (k = j->value.list; k != NULL; k = k->next) {
				BtBencode *l;

				l = bt_bencode_slitem (k);
				if (l->type != BT_BENCODE_TYPE_STRING)
					goto cleanup;

				g_debug ("announce: %s", l->value.string->str);
				list = g_slist_prepend (list, g_strdup (l->value.string->str));
			}

			list = g_slist_reverse (list);
			self->announce_list = g_slist_prepend (self->announce_list, list);
		}

		self->announce_list = g_slist_reverse (self->announce_list);
	}

	/* get and check other keys in the info dict */
	length = bt_bencode_lookup (info, "length");
	files = bt_bencode_lookup (info, "files");
	piece_length = bt_bencode_lookup (info, "piece length");
	pieces = bt_bencode_lookup (info, "pieces");

	if (!pieces || pieces->type != BT_BENCODE_TYPE_STRING)
		goto cleanup;
	if ((length && files) || (!length && !files))
		goto cleanup;
	if (!piece_length || piece_length->type != BT_BENCODE_TYPE_INT)
		goto cleanup;
	if ((length && length->type != BT_BENCODE_TYPE_INT)
	    || (files && files->type != BT_BENCODE_TYPE_LIST))
		goto cleanup;

	self->piece_length = (guint32) piece_length->value.value;

	if (length) {
		/* this torrent is one single file */
		self->size = length->value.value;
		self->files = g_slist_prepend (self->files, NULL);
	}

	if (files) {
		/* we have multiple files in the torrent, so loop through them */
		GSList *i;

		self->size = 0;

		for (i = files->value.list; i != NULL; i = i->next) {
			BtBencode *entry, *length, *path;
			GSList *j;
			gchar **path_strv;
			gchar *full_path;
			gsize k = 0;

			entry = bt_bencode_slitem (i);
			if (entry->type != BT_BENCODE_TYPE_DICT)
				goto cleanup;

			length = bt_bencode_lookup (entry, "length");
			path = bt_bencode_lookup (entry, "path");
			if (!length || length->type != BT_BENCODE_TYPE_INT || !path || path->type != BT_BENCODE_TYPE_LIST)
				goto cleanup;

			path_strv = g_malloc0 ((g_slist_length (path->value.list) + 1) * sizeof (gpointer));

			for (j = path->value.list; j != NULL; j = j->next) {
				if (bt_bencode_slitem (j)->type != BT_BENCODE_TYPE_STRING) {
					g_free (path_strv);
					goto cleanup;
				}

				path_strv[k++] = bt_bencode_slitem (j)->value.string->str;
			}

			self->size += length->value.value;
			full_path = g_build_filenamev (path_strv);
			g_free (path_strv);
			g_debug ("%s", full_path);
			g_free (full_path);
			self->files = g_slist_prepend (self->files, NULL);
		}

		self->files = g_slist_reverse (self->files);
	}

	bt_bencode_destroy (metainfo);
	return TRUE;

  cleanup:
	g_set_error (error, BT_ERROR, BT_ERROR_INVALID_TORRENT, "invalid torrent file");
	bt_bencode_destroy (metainfo);
	return FALSE;
}

static gboolean
bt_torrent_check_peers (BtTorrent *self)
{
	g_return_val_if_fail (BT_IS_TORRENT (self), FALSE);

	return TRUE;
}


gboolean
bt_torrent_start_downloading (BtTorrent *self)
{
	self->check_peers_source = bt_timeout_source_create (self->manager, 5000, (GSourceFunc) bt_torrent_check_peers, self);

	self->announce_source = bt_idle_source_create (self->manager, (GSourceFunc) bt_torrent_announce, self);

	return TRUE;
}

gboolean
bt_torrent_stop_downloading (BtTorrent *self G_GNUC_UNUSED)
{
	return TRUE;
}

gboolean
bt_torrent_add_peer (BtTorrent *self, BtPeer *peer)
{
	self->peers = g_slist_prepend (self->peers, g_object_ref (peer));

	return TRUE;
}



static void
bt_torrent_set_property (GObject *object, guint property, const GValue *value G_GNUC_UNUSED, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtTorrent *self G_GNUC_UNUSED = BT_TORRENT (object);

	switch (property) {

	}
}

static void
bt_torrent_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtTorrent *self = BT_TORRENT (object);

	switch (property) {
	case PROP_NAME:
		g_value_set_string (value, self->name);
		break;

	case PROP_SIZE:
		g_value_set_uint64 (value, self->size);
		break;
	}
}

static void
bt_torrent_init (BtTorrent *torrent)
{
	torrent->manager = NULL;
	torrent->announce = NULL;
	torrent->announce_list = NULL;
	torrent->name = NULL;
	torrent->size = 0;
	torrent->piece_length = 0;
	torrent->files = NULL;
	torrent->check_peers_source = NULL;
	torrent->announce_source = NULL;
	torrent->announce_start = NULL;
	torrent->tracker_current_tier = 0;
	torrent->tracker_current_tracker = 0;
	torrent->tracker_socket = NULL;
	torrent->tracker_interval = -1;
	torrent->tracker_min_interval = -1;
	torrent->tracker_id = NULL;
	torrent->cache = g_string_sized_new (1024);
}

static void
bt_torrent_dispose (GObject *torrent)
{
	BtTorrent *self = BT_TORRENT (torrent);

	if (self->manager == NULL)
		return;

	g_object_unref (self->manager);
	self->manager = NULL;

	((GObjectClass *) bt_torrent_parent_class)->dispose (torrent);
}

static void
bt_torrent_finalize (GObject *torrent)
{
	BtTorrent *self = BT_TORRENT (torrent);

	if (self->tracker_socket)
		bt_torrent_announce_stop (self);

	g_free (self->tracker_id);
	g_free (self->announce);
	g_free (self->name);
	g_string_free (self->cache, TRUE);

	((GObjectClass *) bt_torrent_parent_class)->finalize (torrent);
}

static void
bt_torrent_class_init (BtTorrentClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;

	gclass = G_OBJECT_CLASS (klass);

	gclass->set_property = bt_torrent_set_property;
	gclass->get_property = bt_torrent_get_property;
	gclass->finalize = bt_torrent_finalize;
	gclass->dispose = bt_torrent_dispose;

	pspec = g_param_spec_string ("name",
	                             "torrent name",
	                             "The torrent's name, from the .torrent file",
	                             "",
	                             G_PARAM_READABLE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_NAME, pspec);

	pspec = g_param_spec_uint64 ("size",
	                             "torrent size",
	                             "The size of the torrent in bytes",
	                             0, G_MAXUINT64, 0,
	                             G_PARAM_READABLE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_SIZE, pspec);
}


/**
 * bt_torrent_new:
 *
 * Creates a new BtTorrent object from the .torrent file with the specified filename.
 */

BtTorrent *
bt_torrent_new (BtManager *manager, gchar *filename, GError **error)
{
	BtTorrent *torrent = BT_TORRENT (g_object_new (BT_TYPE_TORRENT, NULL));

	if (!bt_torrent_parse_file (torrent, filename, error)) {
		g_object_unref (torrent);
		return NULL;
	}

	torrent->manager = g_object_ref (manager);

	return torrent;
}
