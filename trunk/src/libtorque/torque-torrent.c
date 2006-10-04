#include <glib.h>

#include "torque-torrent.h"
#include "torque-torrent-file.h"
#include "torque-torrent-peer.h"
#include "torque-bencode.h"

typedef enum {
	PROPERTY_NAME = 1,
	PROPERTY_INFO_HASH,
	PROPERTY_PEER_ID,
	PROPERTY_PIECE_LENGTH,
	PROPERTY_SIZE,
} TorqueTorrentProperty;

struct _TorqueTorrent {
	GObject parent;

	gchar *name;
	gchar info_hash[20];
	gchar info_hash_string[41];
	gchar peer_id[21];
	gsize piece_length;
	gchar *pieces;
	gsize size;
	gchar *announce;
	GSList *announce_list;
	GSList *files;
	gboolean super_seeding;

	gboolean disposed;
};

struct _TorqueTorrentClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (TorqueTorrent, torque_torrent, G_TYPE_OBJECT)

static void
torque_torrent_set_property (GObject *obj, guint property, const GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	TorqueTorrent *self = TORQUE_TORRENT (obj);

	switch (property) {
	case PROPERTY_PEER_ID:
		g_strlcpy (self->peer_id, g_value_get_string (value), 21);
		break;
	}
}

static void
torque_torrent_get_property (GObject *obj, guint property, GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	TorqueTorrent *self = TORQUE_TORRENT (obj);

	switch (property) {
	case PROPERTY_NAME:
		g_value_set_string (value, self->name);
		break;

	case PROPERTY_INFO_HASH:
		g_value_set_string (value, self->info_hash_string);
		break;

	case PROPERTY_PEER_ID:
		g_value_set_string (value, self->peer_id);
		break;

	case PROPERTY_PIECE_LENGTH:
		g_value_set_uint (value, self->piece_length);
		break;

	case PROPERTY_SIZE:
		g_value_set_uint (value, self->size);
		break;
	}
}

static void
torque_torrent_dispose (GObject *obj)
{
	TorqueTorrent *self = TORQUE_TORRENT (obj);

	if (self->disposed)
		return;

	self->disposed = TRUE;

	G_OBJECT_CLASS (torque_torrent_parent_class)->dispose (obj);
}

static void
torque_torrent_finalize (GObject *obj)
{
	TorqueTorrent *self = TORQUE_TORRENT (obj);

	g_free (self->name);
	g_free (self->announce);
	if (self->announce_list) {
		for (GSList *i = self->announce_list; i != NULL; i = i->next) {
			for (GSList *j = i->data; j != NULL; j = j->next)
				g_free (j->data);
			g_slist_free (i->data);
		}
		g_slist_free (self->announce_list);
	}

	g_debug ("finalized torrent");

	G_OBJECT_CLASS (torque_torrent_parent_class)->finalize (obj);
}

static void
torque_torrent_init (TorqueTorrent *self)
{
	self->name = NULL;
	self->announce = NULL;
	self->announce_list = NULL;
	self->files = NULL;
	self->super_seeding = FALSE;
	self->disposed = FALSE;
	return;
}

static void
torque_torrent_class_init (TorqueTorrentClass *klass)
{
	GParamSpec *param;

	GObjectClass *g_class = G_OBJECT_CLASS (klass);

	g_class->get_property = torque_torrent_get_property;
	g_class->set_property = torque_torrent_set_property;
	g_class->dispose = torque_torrent_dispose;
	g_class->finalize = torque_torrent_finalize;

	param = g_param_spec_string ("name", "torrent name", "The torrent's name, as found in the .torrent file. This will also be the default save path.", "", G_PARAM_READABLE);
	g_object_class_install_property (g_class, PROPERTY_NAME, param);

	param = g_param_spec_string ("info-hash", "torrent sha1 info hash", "The torrent's infohash as a hex string.", "", G_PARAM_READABLE);
	g_object_class_install_property (g_class, PROPERTY_INFO_HASH, param);

	param = g_param_spec_string ("peer-id", "torrent peer id", "The torrent's peer id that will be sent to peers.", "", G_PARAM_READWRITE);
	g_object_class_install_property (g_class, PROPERTY_PEER_ID, param);

	param = g_param_spec_uint ("piece-length", "piece length", "The length of a piece in a torrent.", 0, G_MAXSIZE, 0, G_PARAM_READABLE);
	g_object_class_install_property (g_class, PROPERTY_PIECE_LENGTH, param);

	param = g_param_spec_boolean ("super-seeding", "super seeding", "Whether or not this torrent is in super-seeding mode.", FALSE, G_PARAM_READWRITE);
	g_object_class_install_property (g_class, PROPERTY_PIECE_LENGTH, param);

	param = g_param_spec_uint ("size", "torrent size", "The size of the torrent in bytes.", 0, G_MAXSIZE, 0, G_PARAM_READABLE);
	g_object_class_install_property (g_class, PROPERTY_SIZE, param);

	return;
}

TorqueTorrent *
torque_torrent_new (const gchar *uri, GError **error)
{
	TorqueTorrent *torrent;
	TorqueBEncodedData *metainfo, *info, *name, *announce, *announce_list, *nodes, *length, *files, *piece_length, *pieces;
	gchar *contents;
	gsize size;

	g_debug ("creating new torrent");

	torrent = TORQUE_TORRENT (g_object_new (TORQUE_TYPE_TORRENT, NULL));

	g_debug ("getting torrent contents");

	if (!g_file_get_contents (uri, &contents, &size, error)) {
		g_object_unref (torrent);
		return NULL;
	}

	g_debug ("decoding bencoded torrent file");

	metainfo = torque_bencode_decode (contents, error);

	g_free (contents);

	if (!metainfo) {
		g_object_unref (torrent);
		return NULL;
	}

	info = torque_bencode_lookup (metainfo, "info");

	if (!info || info->type != TORQUE_BENCODED_DICT)
		goto cleanup;

	name = torque_bencode_lookup (info, "name");

	if (!name || name->type != TORQUE_BENCODED_STRING)
		goto cleanup;

	torrent->name = g_strdup (name->string->str);
	g_debug ("torrent name: %s", torrent->name);

	torque_info_hash (info, torrent->info_hash);
	torque_hash_to_string (torrent->info_hash, torrent->info_hash_string);
	g_debug ("torrent info hash: %s", torrent->info_hash_string);

	torque_create_peer_id (torrent->peer_id);
	g_debug ("torrent peer id: %s", torrent->peer_id);

	announce = torque_bencode_lookup (metainfo, "announce");
	announce_list = torque_bencode_lookup (metainfo, "announce-list");
	nodes = torque_bencode_lookup (metainfo, "nodes");

	if (announce == NULL && announce_list == NULL && nodes == NULL)
		goto cleanup;

	if (announce) {
		if (announce->type != TORQUE_BENCODED_STRING)
			goto cleanup;

		torrent->announce = g_strdup (announce->string->str);
	}

	if (announce_list) {
		int tier, order;

		if (announce_list->type != TORQUE_BENCODED_LIST)
			goto cleanup;

		tier = order = 0;

		g_debug ("creating announce list");

		for (GSList *i = announce_list->list; i != NULL; i = i->next, tier++, order = 0) {
			GSList *tier_list = NULL;

			if (torque_bencode_slitem (i)->type != TORQUE_BENCODED_LIST)
				goto cleanup;

			for (GSList *j = torque_bencode_slitem (i)->list; j != NULL; j = j->next, order++) {
				if (torque_bencode_slitem (j)->type != TORQUE_BENCODED_STRING)
					goto cleanup;

				tier_list = g_slist_prepend (tier_list, torque_bencode_slitem (j)->string->str);
			}

			tier_list = g_slist_reverse (tier_list);
			torrent->announce_list = g_slist_prepend (torrent->announce_list, tier_list);
		}

		torrent->announce_list = g_slist_reverse (torrent->announce_list);
	}

	if(nodes) {
		g_debug("trackerless torrent, using DHT");
	}

	length = torque_bencode_lookup (info, "length");
	files = torque_bencode_lookup (info, "files");
	piece_length = torque_bencode_lookup (info, "piece length");
	pieces = torque_bencode_lookup (info, "pieces");

	if (!pieces || pieces->type != TORQUE_BENCODED_STRING)
		goto cleanup;
	if ((length && files) || (!length && !files))
		goto cleanup;
	if (!piece_length || piece_length->type != TORQUE_BENCODED_INT)
		goto cleanup;
	if ((length && length->type != TORQUE_BENCODED_INT) || (files && files->type != TORQUE_BENCODED_LIST))
		goto cleanup;

	torrent->pieces = g_strdup (pieces->string->str);
	torrent->piece_length = (guint32) piece_length->value;

	if (length) {
		TorqueTorrentFile *file;

		g_debug ("got a single file, creating it");
		torrent->size = length->value;
		file = torque_torrent_file_new (torrent, torrent->name, torrent->size);
		torrent->files = g_slist_prepend (torrent->files, file);
	}

	if (files) {
		GSList *i;

		g_debug ("got multiple files, building list");
		torrent->size = 0;

		for (i = files->list; i != NULL; i = i->next) {
			TorqueTorrentFile *file;
			TorqueBEncodedData *entry, *length, *path;
			GSList *j;
			gchar **path_strv;
			gchar *full_path;
			gsize k;

			entry = torque_bencode_slitem (i);

			if (entry->type != TORQUE_BENCODED_DICT)
				goto cleanup;

			length = torque_bencode_lookup (entry, "length");
			path = torque_bencode_lookup (entry, "path");

			if (!length || length->type != TORQUE_BENCODED_INT || !path || path->type != TORQUE_BENCODED_LIST)
				goto cleanup;

			path_strv = g_malloc0 ((g_slist_length (path->list) + 1) * sizeof (gpointer));
			k = 0;

			for (j = path->list; j != NULL; j = j->next) {
				if (torque_bencode_slitem (j)->type != TORQUE_BENCODED_STRING) {
					g_free (path_strv);
					goto cleanup;
				}
				path_strv[k++] = torque_bencode_slitem (j)->string->str;
			}

			torrent->size += length->value;

			full_path = g_build_filenamev (path_strv);

			g_free (path_strv);

			g_debug ("creating file for %s", full_path);
			file = torque_torrent_file_new (torrent, full_path, length->value);

			g_free (full_path);

			torrent->files = g_slist_prepend (torrent->files, file);
		}

		torrent->files = g_slist_reverse (torrent->files);
	}

	torque_bencode_destroy (metainfo);
	g_debug ("done creating torrent");

	return torrent;

cleanup:
	g_set_error (error, TORQUE_ERROR, TORQUE_ERROR_INVALID_TORRENT, "invalid torrent file");
	g_object_unref (torrent);
	return NULL;
}

const gchar *
torque_torrent_get_info_hash (TorqueTorrent *torrent)
{
	g_return_val_if_fail (torrent, NULL);

	return torrent->info_hash_string;
}

const gchar *
torque_torrent_get_name (TorqueTorrent *torrent)
{
	g_return_val_if_fail (torrent, NULL);

	return torrent->name;
}
