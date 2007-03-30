/**
 * bt-torrent.c
 *
 * Copyright 2007 Samuel Cormier-Iijima <sciyoshi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <gnet.h>

#include "bt-torrent.h"
#include "bt-bencode.h"
#include "bt-manager.h"
#include "bt-torrent-file.h"
#include "bt-utils.h"

enum {
	BT_TORRENT_PROPERTY_NAME = 1,
	BT_TORRENT_PROPERTY_SIZE,
	BT_TORRENT_PROPERTY_MANAGER
};

enum {
	BT_TORRENT_SIGNAL_TRACKER_UPDATED,
	BT_TORRENT_NUM_SIGNALS
};

static guint bt_torrent_signals[BT_TORRENT_NUM_SIGNALS] = {0, };

struct _BtTorrent {
	GObject    parent;

	/* manager for this torrent */	
	BtManager *manager;
	
	/* name of this torrent */
	gchar     *name;
	
	/* size of the torrent in bytes */
	guint64    size;
	
	/* length of each piece */
	guint32    piece_length;
	
	/* infohash as a 20-byte binary string */
	gchar     *infohash;
	
	/* infohash as a 40-byte human-readable string */
	gchar     *infohash_string;
	
	gchar     *announce;
	
	GSList    *announce_list;
	
	guint      num_pieces;
	
	gdouble    completion;
	
	guint      num_blocks;
	
	guint      block_size;
	
	gchar     *bitfield;
	
	GArray    *files;
	
	GList     *peers;

	/* tracker */
	GConnHttp *tracker_connection;
	guint32    tracker_interval;
	guint32    tracker_min_interval;
	gchar     *tracker_id;
};

struct _BtTorrentClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (BtTorrent, bt_torrent, G_TYPE_OBJECT)

static gboolean
bt_torrent_announce_http_parse_response (BtTorrent *torrent, gchar *buf, gsize len)
{
	GError *error;
	BtBencode *response, *failure, *warning, *interval, *tracker_id, *peers;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), FALSE);
	g_return_val_if_fail (buf != NULL, FALSE);

	error = NULL;

	if (!(response = bt_bencode_decode (buf, len, &error))) {
		g_warning ("could not decode tracker response: %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	failure = bt_bencode_lookup (response, "failure reason");

	if (failure) {
		if (failure->type == BT_BENCODE_TYPE_STRING)
			g_warning ("tracker sent error: %s", failure->string->str);
		return FALSE;
	}

	warning = bt_bencode_lookup (response, "warning message");

	if (warning && warning->type == BT_BENCODE_TYPE_STRING)
		g_warning ("tracker sent warning: %s", warning->string->str);

	interval = bt_bencode_lookup (response, "interval");

	if (interval && interval->type == BT_BENCODE_TYPE_INT) {
		torrent->tracker_interval = interval->value;
		g_debug ("tracker announce interval: %d", torrent->tracker_interval);
	}

	interval = bt_bencode_lookup (response, "min interval");

	if (interval && interval->type == BT_BENCODE_TYPE_INT) {
		torrent->tracker_min_interval = interval->value;
		g_debug ("tracker announce minimum interval: %d", torrent->tracker_min_interval);
	}

	tracker_id = bt_bencode_lookup (response, "tracker id");

	if (tracker_id && tracker_id->type == BT_BENCODE_TYPE_STRING) {
		torrent->tracker_id = g_strdup (tracker_id->string->str);
		g_debug ("tracker id: %s", torrent->tracker_id);
	}

	peers = bt_bencode_lookup (response, "peers");

	if (peers && peers->type == BT_BENCODE_TYPE_STRING) {
		int num, i;

		if (peers->string->len % 6 != 0)
			g_warning ("invalid peers string");

		num = peers->string->len / 6;

		for (i = 0; i < num; i++) {
			GInetAddr *address;
			BtPeer *peer;
			
			address = gnet_inetaddr_new_bytes (peers->string->str + i * 6, 4);
			
			gnet_inetaddr_set_port (address, g_ntohs (*((gushort *) (peers->string->str + i * 6 + 4))));
			
			peer = bt_peer_new_outgoing (torrent->manager, torrent, address);
			
			bt_torrent_add_peer (torrent, peer);
			
			//g_object_unref (peer);
			gnet_inetaddr_unref (address);
		}
	}

	bt_bencode_destroy (response);

	return TRUE;
}

/**
 * bt_torrent_tracker_stop_announce:
 * @torrent: the torrent
 *
 * Stops any currently running announce.
 */
void
bt_torrent_tracker_stop_announce (BtTorrent *torrent)
{
	g_return_if_fail (BT_IS_TORRENT (torrent));

	if (torrent->tracker_connection == NULL)
		return;
	
	gnet_conn_http_delete (torrent->tracker_connection);
	
	torrent->tracker_connection = NULL;
	
	return;
}

static gboolean
bt_torrent_tracker_stop_announce_source (gpointer data)
{
	BtTorrent *torrent;
	
	g_return_val_if_fail (BT_IS_TORRENT (data), FALSE);
	
	torrent = BT_TORRENT (data);
	
	bt_torrent_tracker_stop_announce (torrent);
	
	return FALSE; 
}

static void
bt_torrent_tracker_http_response (GConnHttp *connection G_GNUC_UNUSED, GConnHttpEvent *event, gpointer data)
{
	GConnHttpEventResponse *event_response;
	GConnHttpEventData *event_data;
	BtTorrent *torrent;
	gchar *buf;
	gsize len;
	
	g_return_if_fail (data != NULL && event != NULL);
	
	torrent = BT_TORRENT (data);
	
	switch (event->type) {
	case GNET_CONN_HTTP_RESOLVED:
		g_debug ("tracker address resolved");
		break;
		
	case GNET_CONN_HTTP_RESPONSE:
		event_response = (GConnHttpEventResponse *) event;
		g_debug ("tracker sent response with code: %d", event_response->response_code);
		break;

	case GNET_CONN_HTTP_DATA_COMPLETE:
		event_data = (GConnHttpEventData *) event;
		gnet_conn_http_steal_buffer (connection, &buf, &len);
		bt_torrent_announce_http_parse_response (torrent, buf, len);
		g_free (buf);
		g_idle_add (bt_torrent_tracker_stop_announce_source, torrent);
		break;

	case GNET_CONN_HTTP_ERROR:
		g_warning ("tracker conection error");
		g_idle_add (bt_torrent_tracker_stop_announce_source, torrent);
		
	default:
		break;
	}

	return;
}

static void
bt_torrent_tracker_announce_single (BtTorrent *torrent, const gchar *tracker)
{
	gchar *query, *tmp;

	g_return_if_fail (BT_IS_TORRENT (torrent));
	g_return_if_fail (tracker != NULL);

	if (torrent->tracker_connection != NULL) {
		g_debug ("already trying to update tracker");
		return;
	}

	/* build query */
	tmp = bt_url_encode (torrent->infohash, 20);

	query = g_strdup_printf ("%s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%" G_GINT64_FORMAT "&compact=1&event=%s&numwant=%d",
	                         torrent->announce,
	                         tmp,
	                         bt_manager_get_peer_id (torrent->manager),
	                         bt_manager_get_port (torrent->manager),
	                         0,
	                         0,
	                         torrent->size,
	                         "started",
	                         30);

	g_debug ("hitting tracker with query %s", query);

	g_free (tmp);

	torrent->tracker_connection = gnet_conn_http_new ();

	if (!gnet_conn_http_set_escaped_uri (torrent->tracker_connection, query)) {
		gnet_conn_http_delete (torrent->tracker_connection);
		torrent->tracker_connection = NULL;
		g_warning ("could not accept uri");
		return;
	}

	gnet_conn_http_run_async (torrent->tracker_connection, bt_torrent_tracker_http_response, torrent);
	
	return;
}

/**
 * bt_torrent_tracker_announce:
 * @torrent: the torrent
 *
 * Starts a tracker announce.
 */
void
bt_torrent_tracker_announce (BtTorrent *torrent)
{
	g_return_if_fail (BT_IS_TORRENT (torrent));

	bt_torrent_tracker_announce_single (torrent, torrent->announce);
}

static gboolean
bt_torrent_parse_file (BtTorrent *torrent, const gchar *filename, GError **error)
{
	BtBencode *metainfo, *info, *announce, *announce_list, *name, *length, *files, *pieces, *piece_length;
	gchar *contents;
	gsize len;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), FALSE);
	g_return_val_if_fail (filename != NULL, FALSE);

	/* read the contents of the torrent file */
	if (!g_file_get_contents (filename, &contents, &len, error))
		return FALSE;

	/* decode the bencoded file into our own structure */
	metainfo = bt_bencode_decode (contents, len, error);

	/* we won't need the contents anymore */
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

	torrent->name = g_strdup (name->string->str);
	g_debug ("torrent name: %s", torrent->name);

	/* check the info hash of this torrent */
	torrent->infohash = bt_get_infohash (info);
	torrent->infohash_string = bt_hash_to_string (torrent->infohash);
	g_debug ("torrent info hash: %s", torrent->infohash_string);

	/* find out how to announce to the tracker(s) */
	/* TODO: PROTOCOL: should we be more lenient about missing announce if there is an announce-list? */
	announce = bt_bencode_lookup (metainfo, "announce");
	announce_list = bt_bencode_lookup (metainfo, "announce-list");

	if (announce == NULL)
		goto cleanup;

	torrent->announce = g_strdup (announce->string->str);

	if (announce_list) {
		/* the announce-list is a list of lists of strings (doubly nested) */
		GSList *i;

		torrent->announce_list = NULL;

		if (announce_list->type != BT_BENCODE_TYPE_LIST)
			goto cleanup;

		for (i = announce_list->list; i != NULL; i = i->next) {
			GSList *list, *k;
			BtBencode *j;

			/* initialize the new tier */
			list = NULL;

			j = bt_bencode_slitem (i);
			if (j->type != BT_BENCODE_TYPE_LIST)
				goto cleanup;

			for (k = j->list; k != NULL; k = k->next) {
				BtBencode *l;

				l = bt_bencode_slitem (k);
				if (l->type != BT_BENCODE_TYPE_STRING)
					goto cleanup;

				/* add it to this tier */
				g_debug ("announce: %s", l->string->str);
				list = g_slist_prepend (list, g_strdup (l->string->str));
			}

			/* add the tier to the announce list */
			list = g_slist_reverse (list);
			torrent->announce_list = g_slist_prepend (torrent->announce_list, list);
		}

		torrent->announce_list = g_slist_reverse (torrent->announce_list);
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

	torrent->piece_length = (guint32) piece_length->value;

	torrent->size = length->value;

	if (length) {
		/* this torrent is one single file */
		torrent->size = length->value;

		BtTorrentFile file = {g_strdup (torrent->name), torrent->size, 0, 0};

		g_array_append_val (torrent->files, file);
	}

	if (files) {
		/* we have multiple files in the torrent, so loop through them */
		GSList *i;

		torrent->size = 0;

		for (i = files->list; i != NULL; i = i->next) {
			BtBencode *entry, *length, *path;
			GSList *j;
			gchar **path_strv;
			gchar *full_path;
			BtTorrentFile file = {NULL, 0, 0, 0};
			gsize k = 0;

			entry = bt_bencode_slitem (i);
			if (entry->type != BT_BENCODE_TYPE_DICT)
				goto cleanup;

			length = bt_bencode_lookup (entry, "length");
			path = bt_bencode_lookup (entry, "path");
			if (!length || length->type != BT_BENCODE_TYPE_INT || !path || path->type != BT_BENCODE_TYPE_LIST)
				goto cleanup;

			path_strv = g_malloc0 ((g_slist_length (path->list) + 1) * sizeof (gpointer));

			for (j = path->list; j != NULL; j = j->next) {
				if (bt_bencode_slitem (j)->type != BT_BENCODE_TYPE_STRING) {
					g_free (path_strv);
					goto cleanup;
				}

				path_strv[k++] = bt_bencode_slitem (j)->string->str;
			}

			torrent->size += length->value;
			full_path = g_build_filenamev (path_strv);
			g_free (path_strv);
			g_debug ("%s", full_path);

			file.size = length->value;
			file.name = full_path;

			g_array_append_val (torrent->files, file);

			file.offset += file.size;
		}
	}

	torrent->num_pieces = (torrent->size + torrent->piece_length - 1) / torrent->piece_length;

	torrent->bitfield = g_malloc0 ((torrent->num_pieces + 7) /  8);

	torrent->block_size = MIN (torrent->piece_length, 16384);

	torrent->num_blocks = (torrent->size + torrent->block_size - 1) / torrent->block_size;

	bt_bencode_destroy (metainfo);
	return TRUE;

cleanup:
	g_set_error (error, BT_ERROR, BT_ERROR_INVALID_TORRENT, "invalid torrent file");
	bt_bencode_destroy (metainfo);
	return FALSE;
}

/**
 * bt_torrent_get_infohash:
 * @torrent: the torrent
 *
 * Get the infohash of this torrent as a 20-byte array.
 *
 * Returns: the infohash of the torrent, which is a pointer to an internal string and should
 *   not be modified or freed.
 */
const gchar *
bt_torrent_get_infohash (BtTorrent *torrent)
{
	g_return_val_if_fail (BT_IS_TORRENT (torrent), NULL);

	return torrent->infohash;
}

/**
 * bt_torrent_get_infohash_string:
 * @torrent: the torrent
 *
 * Get the infohash of this torrent as a 40-byte string.
 *
 * Returns: the infohash of the torrent as a string, which is a pointer to an internal
 *   string and should not be modified or freed.
 */
const gchar *
bt_torrent_get_infohash_string (BtTorrent *torrent)
{
	g_return_val_if_fail (BT_IS_TORRENT (torrent), NULL);

	return torrent->infohash_string;
}

/**
 * bt_torrent_get_name:
 * @torrent: the torrent
 *
 * Get the name of this torrent.
 *
 * Returns: the name of the torrent, which is a pointer to an internal string and should
 *   not be modified or freed.
 */
const gchar *
bt_torrent_get_name (BtTorrent *torrent)
{
	g_return_val_if_fail (BT_IS_TORRENT (torrent), NULL);

	return torrent->name;
}

/**
 * bt_torrent_get_size:
 * @torrent: the torrent
 *
 * Get the size of this torrent in bytes.
 *
 * Returns: the size of the torrent in bytes as a guint64.
 */
guint64
bt_torrent_get_size (BtTorrent *torrent)
{
	g_return_val_if_fail (BT_IS_TORRENT (torrent), 0);

	return torrent->size;
}

/**
 * bt_torrent_start:
 * @torrent: the torrent
 *
 * Start running this torrent.
 */
void
bt_torrent_start (BtTorrent *torrent)
{
	g_return_if_fail (BT_IS_TORRENT (torrent));
	
	bt_torrent_tracker_announce (torrent);
}

/**
 * bt_torrent_stop:
 * @torrent: the torrent
 *
 * Stop this torrent.
 */
void
bt_torrent_stop (BtTorrent *torrent)
{
	g_return_if_fail (BT_IS_TORRENT (torrent));
}

/**
 * bt_torrent_pause:
 * @torrent: the torrent
 *
 * Pause this torrent. Peers will stay connected, but no data will
 * be sent.
 */
void
bt_torrent_pause (BtTorrent *torrent)
{
	g_return_if_fail (BT_IS_TORRENT (torrent));
}

/**
 * bt_torrent_add_peer:
 * @torrent: the torrent
 * @peer: the peer
 *
 * Add a peer to this torrent.
 */
void
bt_torrent_add_peer (BtTorrent *torrent, BtPeer *peer)
{
	g_return_if_fail (BT_IS_TORRENT (torrent));
	g_return_if_fail (BT_IS_PEER (peer));
	
	g_object_ref (peer);
}

BtTorrent *
bt_torrent_new (BtManager *manager, gchar *filename, GError **error)
{
	GObject *torrent = g_object_new (BT_TYPE_TORRENT, "manager", manager, NULL);
	
	bt_torrent_parse_file (BT_TORRENT (torrent), filename, error);
	
	return BT_TORRENT (torrent);
}

static void
bt_torrent_dispose (GObject *object)
{
	BtTorrent *self = BT_TORRENT (object);
	
	if (self->name == NULL)
		return;
	
	g_free (self->name);
	
	G_OBJECT_CLASS (bt_torrent_parent_class)->dispose (object);
	
	return;
}

static void
bt_torrent_finalize (GObject *object)
{
	G_OBJECT_CLASS (bt_torrent_parent_class)->finalize (object);
	
	return;
}

static void
bt_torrent_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec)
{
	BtTorrent *self = BT_TORRENT (object);
	
	switch (property) {
	case BT_TORRENT_PROPERTY_NAME:
		self->name = g_value_dup_string (value);
		break;

	case BT_TORRENT_PROPERTY_MANAGER:
		self->manager = BT_MANAGER (g_value_dup_object (value));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
	
	return;
}

static void
bt_torrent_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec)
{
	BtTorrent *self = BT_TORRENT (object);
	
	switch (property) {
	case BT_TORRENT_PROPERTY_NAME:
		g_value_set_string (value, self->name);
		break;

	case BT_TORRENT_PROPERTY_SIZE:
		g_value_set_uint64 (value, self->size);
		break;
	
	case BT_TORRENT_PROPERTY_MANAGER:
		g_value_set_object (value, self->manager);
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
	
	return;
}

static void
bt_torrent_init (BtTorrent *torrent)
{
	torrent->files = g_array_new (FALSE, TRUE, sizeof (BtTorrentFile));
	torrent->peers = NULL;

	return;
}

static void
bt_torrent_class_init (BtTorrentClass *torrent_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (torrent_class);
	GParamSpec *pspec;

	object_class->dispose = bt_torrent_dispose;
	object_class->finalize = bt_torrent_finalize;
	object_class->set_property = bt_torrent_set_property;
	object_class->get_property = bt_torrent_get_property;
	
	/**
	 * BtTorrent:name:
	 *
	 * The name of this torrent, retrieved from the .torrent metainfo file.
	 */
	pspec = g_param_spec_string ("name",
	                             "torrent name",
	                             "The torrent's name as gotten from the .torrent file",
	                             "invalid torrent",
	                             G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK);
	
	g_object_class_install_property (object_class, BT_TORRENT_PROPERTY_NAME, pspec);

	/**
	 * BtTorrent:size:
	 *
	 * The size of this torrent in bytes as a 64-bit integer.
	 */
	pspec = g_param_spec_uint64 ("size",
	                             "torrent size",
	                             "The torrent's size as gotten from the .torrent file",
	                             0,
	                             G_MAXUINT64,
	                             0,
	                             G_PARAM_READABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK);
	
	g_object_class_install_property (object_class, BT_TORRENT_PROPERTY_SIZE, pspec);

	/**
	 * BtTorrent:manager:
	 *
	 * The #BtManager that controls this torrent.
	 */
	pspec = g_param_spec_object ("manager",
	                             "torrent manager",
	                             "This torrent's manager",
	                             BT_TYPE_MANAGER,
	                             G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK | G_PARAM_CONSTRUCT_ONLY);
	
	g_object_class_install_property (object_class, BT_TORRENT_PROPERTY_MANAGER, pspec);
	
	/**
	 * BtTorrent::tracker-updated:
	 *
	 * Emitted when the tracker is updated.
	 */
	bt_torrent_signals[BT_TORRENT_SIGNAL_TRACKER_UPDATED] =
		g_signal_newv ("tracker-updated",
		               G_TYPE_FROM_CLASS (object_class),
		               G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		               NULL,
		               NULL,
		               NULL,
		               g_cclosure_marshal_VOID__VOID,
		               G_TYPE_NONE,
		               0,
		               NULL);
	
	return;
}
