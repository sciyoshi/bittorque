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
#include "bt-utils.h"

#define BT_TORRENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), BT_TYPE_TORRENT, BtTorrentPrivate))

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

typedef struct _BtTorrentPrivate BtTorrentPrivate;

struct _BtTorrentPrivate {
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

	/* tracker to announce to in case of single tracker, or currently-announcing tracker */	
	gchar     *announce;
	
	/* list of tiers of trackers - multitracker extension */
	GSList    *announce_list;
	
	/* number of pieces in the torrent */
	guint      num_pieces;
	
	/* percent download completion */
	gdouble    completion;
	
	/* number of blocks */
	guint      num_blocks;
	
	/* block request size */
	guint      block_size;
	
	/* bitfield of pieces that we have */
	gchar     *bitfield;

	/* array of 20-byte hashes for each piece */
	gchar     *pieces;

	/* an array of files in this torrent */
	GArray    *files;
	
	/* a list of peers */
	GList     *peers;

	/* tracker */
	GConnHttp *tracker_connection;
	guint32    tracker_interval;
	guint32    tracker_min_interval;
	gchar     *tracker_id;
};

G_DEFINE_TYPE (BtTorrent, bt_torrent, G_TYPE_OBJECT)

static gboolean
bt_torrent_announce_http_parse_response (BtTorrent *torrent, gchar *buf, gsize len)
{
	BtTorrentPrivate *priv;
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

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	if (interval && interval->type == BT_BENCODE_TYPE_INT) {
		priv->tracker_interval = interval->value;
		g_debug ("tracker announce interval: %d", priv->tracker_interval);
	}

	interval = bt_bencode_lookup (response, "min interval");

	if (interval && interval->type == BT_BENCODE_TYPE_INT) {
		priv->tracker_min_interval = interval->value;
		g_debug ("tracker announce minimum interval: %d", priv->tracker_min_interval);
	}

	tracker_id = bt_bencode_lookup (response, "tracker id");

	if (tracker_id && tracker_id->type == BT_BENCODE_TYPE_STRING) {
		priv->tracker_id = g_strdup (tracker_id->string->str);
		g_debug ("tracker id: %s", priv->tracker_id);
	}

	peers = bt_bencode_lookup (response, "peers");

	if (peers) {
		if (peers->type == BT_BENCODE_TYPE_STRING) {
			int num, i;

			if (peers->string->len % 6 != 0)
				g_warning ("invalid peers string");

			num = peers->string->len / 6;

			for (i = 0; i < num; i++) {
				GInetAddr *address;
				BtPeer *peer;
				
				address = gnet_inetaddr_new_bytes (peers->string->str + i * 6, 4);
				
				gnet_inetaddr_set_port (address, g_ntohs (*((gushort *) (peers->string->str + i * 6 + 4))));
				
				peer = bt_peer_new_outgoing (priv->manager, torrent, address);
				
				bt_torrent_add_peer (torrent, peer);
				
				g_object_unref (G_OBJECT (peer));
				gnet_inetaddr_unref (address);
			}
		} else if (peers->type == BT_BENCODE_TYPE_LIST) {
			// if tracker didn't support compact=1
			GSList *i;
			for (i = peers->list; i != NULL; i = i->next) {
				GInetAddr *address;
				BtPeer *peer;
				BtBencode *j, *ip, *port;

				j = bt_bencode_slitem (i);
				if (j->type != BT_BENCODE_TYPE_DICT)
					continue;

				ip = bt_bencode_lookup (j, "ip");
				if (!ip || ip->type != BT_BENCODE_TYPE_STRING)
					continue;

				port = bt_bencode_lookup (j, "port");
				if (!port || port->type != BT_BENCODE_TYPE_INT)
					continue;

				// FIXME: blocks if ip is a dns name
				address = gnet_inetaddr_new (ip->string->str, 4);

				gnet_inetaddr_set_port (address, port->value);

				peer = bt_peer_new_outgoing (priv->manager, torrent, address);
				
				bt_torrent_add_peer (torrent, peer);
				
				g_object_unref (G_OBJECT (peer));
				gnet_inetaddr_unref (address);
			}
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
	BtTorrentPrivate *priv;

	g_return_if_fail (BT_IS_TORRENT (torrent));

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	if (priv->tracker_connection == NULL)
		return;
	
	gnet_conn_http_delete (priv->tracker_connection);
	
	priv->tracker_connection = NULL;
	
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
	BtTorrentPrivate *priv;
	GConnHttpEventResponse *event_response;
	GConnHttpEventData *event_data;
	BtTorrent *torrent;
	gchar *buf;
	gsize len;
	
	g_return_if_fail (data != NULL && event != NULL);

	torrent = BT_TORRENT (data);
	priv = BT_TORRENT_GET_PRIVATE (torrent);
	
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
	BtTorrentPrivate *priv;
	gchar *query, *tmp;

	g_return_if_fail (BT_IS_TORRENT (torrent));
	g_return_if_fail (tracker != NULL);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	if (priv->tracker_connection != NULL) {
		g_debug ("already trying to update tracker");
		return;
	}

	/* build query */
	tmp = bt_url_encode (priv->infohash, 20);

	query = g_strdup_printf ("%s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%" G_GINT64_FORMAT "&compact=1&no_peer_id=1&event=%s&numwant=%d",
	                         priv->announce,
	                         tmp,
	                         bt_manager_get_peer_id (priv->manager),
	                         bt_manager_get_port (priv->manager),
	                         0,
	                         0,
	                         priv->size,
	                         "started",
	                         30);

	g_debug ("hitting tracker with query %s", query);

	g_free (tmp);

	priv->tracker_connection = gnet_conn_http_new ();

	if (!gnet_conn_http_set_escaped_uri (priv->tracker_connection, query)) {
		gnet_conn_http_delete (priv->tracker_connection);
		priv->tracker_connection = NULL;
		g_warning ("could not accept uri");
	}
	else
		gnet_conn_http_run_async (priv->tracker_connection, bt_torrent_tracker_http_response, torrent);

	g_free (query);

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
	BtTorrentPrivate *priv;

	g_return_if_fail (BT_IS_TORRENT (torrent));

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	bt_torrent_tracker_announce_single (torrent, priv->announce);
}

static gboolean
bt_torrent_parse_file (BtTorrent *torrent, const gchar *filename, GError **error)
{
	BtTorrentPrivate *priv;
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

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	priv->name = g_strdup (name->string->str);
	g_debug ("torrent name: %s", priv->name);

	/* check the info hash of this torrent */
	priv->infohash = bt_get_infohash (info);
	priv->infohash_string = bt_hash_to_string (priv->infohash);
	g_debug ("torrent info hash: %s", priv->infohash_string);

	/* find out how to announce to the tracker(s) */
	/* TODO: PROTOCOL: should we be more lenient about missing announce if there is an announce-list? */
	announce = bt_bencode_lookup (metainfo, "announce");
	announce_list = bt_bencode_lookup (metainfo, "announce-list");

	if (announce == NULL)
		goto cleanup;

	priv->announce = g_strdup (announce->string->str);

	g_debug ("torrent announce url: %s", priv->announce);

	if (announce_list) {
		/* the announce-list is a list of lists of strings (doubly nested) */
		GSList *i;

		priv->announce_list = NULL;

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
			priv->announce_list = g_slist_prepend (priv->announce_list, list);
		}

		priv->announce_list = g_slist_reverse (priv->announce_list);
	}

	/* get and check other keys in the info dict */
	length = bt_bencode_lookup (info, "length");
	files = bt_bencode_lookup (info, "files");
	piece_length = bt_bencode_lookup (info, "piece length");
	pieces = bt_bencode_lookup (info, "pieces");

	if (!pieces || pieces->type != BT_BENCODE_TYPE_STRING
		|| (pieces->string->len % 20) != 0)
		goto cleanup;
	if ((length && files) || (!length && !files))
		goto cleanup;
	if (!piece_length || piece_length->type != BT_BENCODE_TYPE_INT)
		goto cleanup;
	if ((length && length->type != BT_BENCODE_TYPE_INT)
		|| (files && files->type != BT_BENCODE_TYPE_LIST))
		goto cleanup;

	priv->piece_length = (guint32) piece_length->value;

	if (length) {
		/* this torrent is one single file */
		BtTorrentFile file = {g_strdup (priv->name), length->value, 0, 0};

		priv->size = length->value;

		g_array_append_val (priv->files, file);
	}

	if (files) {
		/* we have multiple files in the torrent, so loop through them */
		GSList *i;

		priv->size = 0;

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

			priv->size += length->value;
			full_path = g_build_filenamev (path_strv);
			g_free (path_strv);
			g_debug ("%s", full_path);

			file.size = length->value;
			file.name = full_path;

			g_array_append_val (priv->files, file);

			file.offset += file.size;
		}
	}

	priv->num_pieces = (priv->size + priv->piece_length - 1) / priv->piece_length;

	priv->pieces = g_memdup (pieces->string->str, pieces->string->len);

	priv->bitfield = g_malloc0 ((priv->num_pieces + 7) /  8);

	priv->block_size = MIN (priv->piece_length, 16384);

	priv->num_blocks = (priv->size + priv->block_size - 1) / priv->block_size;

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
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), NULL);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->infohash;
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
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), NULL);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->infohash_string;
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
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), NULL);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->name;
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
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), 0);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->size;
}

/**
 * bt_torrent_get_num_pieces:
 * @torrent: the torrent
 *
 * Get the number of pieces for the torrent.
 *
 * Returns: the number of pieces for the torrent as a guint.
 */
guint
bt_torrent_get_num_pieces (BtTorrent *torrent)
{
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), 0);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->num_pieces;
}

/**
 * bt_torrent_get_piece_length:
 * @torrent: the torrent
 *
 * Get the piece length for the torrent.
 *
 * Returns: the piece length for the torrent as a guint32.
 */
guint32
bt_torrent_get_piece_length (BtTorrent *torrent)
{
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), 0);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->piece_length;
}

/**
 * bt_torrent_get_piece_length_extended:
 * @torrent: the torrent
 * @piece: the piece index
 *
 * Get the piece length for the piece index.
 *
 * Returns: the piece length for the piece index as a guint32.
 * Always the same as @bt_torrent_get_piece_length except for the last piece,
 * which might be slighty less.
 */
guint32
bt_torrent_get_piece_length_extended (BtTorrent *torrent, guint piece)
{
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), 0);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	if (piece == (priv->num_pieces - 1))
		return priv->size - ((priv->num_pieces - 1) * priv->piece_length);
	else
		return priv->piece_length;
}

/**
 * bt_torrent_get_piece_hash:
 * @torrent: the torrent
 * @piece: the piece index
 *
 * Get the infohash of the piece as a 20-byte array.
 *
 * Returns: the 20-byte infohash of the piece, 
 * which is a pointer to an internal string and should not be modified or freed.
 */
const gchar *
bt_torrent_get_piece_hash (BtTorrent* torrent, guint piece)
{
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), NULL);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	g_return_val_if_fail (piece < priv->num_pieces, NULL);

	return &priv->pieces[20 * piece];
}

/**
 * bt_torrent_get_num_blocks:
 * @torrent: the torrent
 *
 * Get the number of blocks for the torrent.
 *
 * Returns: the number of blocks for the torrent as a guint.
 */
guint
bt_torrent_get_num_blocks (BtTorrent *torrent)
{
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), 0);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->num_blocks;
}

/**
 * bt_torrent_get_block_size:
 * @torrent: the torrent
 *
 * Get the block size for the torrent.
 *
 * Returns: the block size for the torrent as a guint.
 */
guint
bt_torrent_get_block_size (BtTorrent *torrent)
{
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), 0);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->block_size;
}

/**
 * bt_torrent_get_file:
 * @torrent: the torrent
 * @index: the file index
 *
 * Get a file by index.
 *
 * Returns: the file specified by @index.
 */
const BtTorrentFile *
bt_torrent_get_file (BtTorrent *torrent, guint index)
{
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), NULL);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	g_return_val_if_fail (index < priv->files->len, NULL);

	return &g_array_index (priv->files, BtTorrentFile, index);
}

/**
 * bt_torrent_get_num_files:
 * @torrent: the torrent
 *
 * Get the number of files for the torrent.
 *
 * Returns: the number of files for the torrent as a guint.
 */
guint
bt_torrent_get_num_files (BtTorrent *torrent)
{
	BtTorrentPrivate *priv;

	g_return_val_if_fail (BT_IS_TORRENT (torrent), 0);

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	return priv->files->len;
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
	BtTorrentPrivate* priv;

	g_return_if_fail (BT_IS_TORRENT (torrent));
	g_return_if_fail (BT_IS_PEER (peer));

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	g_return_if_fail (!g_list_find (priv->peers, peer));

	priv->peers = g_list_append (priv->peers, g_object_ref (peer));
}

BtTorrent *
bt_torrent_new (BtManager *manager, gchar *filename, GError **error)
{
	GObject *torrent;
	
	g_return_val_if_fail (BT_IS_MANAGER (manager), NULL);
	
	torrent = g_object_new (BT_TYPE_TORRENT, "manager", manager, NULL);
	
	bt_torrent_parse_file (BT_TORRENT (torrent), filename, error);
	
	return BT_TORRENT (torrent);
}

static void
bt_torrent_dispose (GObject *object)
{
	BtTorrent *torrent;
	BtTorrentPrivate *priv;

	torrent = BT_TORRENT (object);
	priv = BT_TORRENT_GET_PRIVATE (torrent);

	if (priv->name == NULL)
		return;

	g_free (priv->name);

	g_free (priv->infohash);
	g_free (priv->infohash_string);
	g_free (priv->announce);
	// FIXME: free contents of announce_list
	g_slist_free (priv->announce_list);
	g_free (priv->bitfield);
	g_free (priv->pieces);
	g_array_free (priv->files, TRUE);

	if (priv->peers != NULL)
	{
		GList *i = NULL;
		for (i = priv->peers; i != NULL; i = i->next) {
			g_object_unref (G_OBJECT (i->data));
		}
		g_list_free (priv->peers);
	}

	if (priv->tracker_id != NULL)
		g_free (priv->tracker_id);

	// delete tracker_connection
	bt_torrent_tracker_stop_announce (torrent);

	g_object_unref (torrent->io);

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
	BtTorrentPrivate *priv;

	priv = BT_TORRENT_GET_PRIVATE (BT_TORRENT (object));
	
	switch (property) {
	case BT_TORRENT_PROPERTY_NAME:
		priv->name = g_value_dup_string (value);
		break;

	case BT_TORRENT_PROPERTY_MANAGER:
		bt_remove_weak_pointer (G_OBJECT (priv->manager), (gpointer)&priv->manager);
		priv->manager = BT_MANAGER (g_value_get_pointer (value));
		bt_add_weak_pointer (G_OBJECT (priv->manager), (gpointer)&priv->manager);
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
	
	return;
}

static void
bt_torrent_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec)
{
	BtTorrentPrivate *priv;

	priv = BT_TORRENT_GET_PRIVATE (BT_TORRENT (object));

	switch (property) {
	case BT_TORRENT_PROPERTY_NAME:
		g_value_set_string (value, priv->name);
		break;

	case BT_TORRENT_PROPERTY_SIZE:
		g_value_set_uint64 (value, priv->size);
		break;
	
	case BT_TORRENT_PROPERTY_MANAGER:
		// FIXME: this might leave a dangling pointer in value
		g_value_set_pointer (value, priv->manager);
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
	
	return;
}

static void
bt_torrent_init (BtTorrent *torrent)
{
	BtTorrentPrivate *priv;

	priv = BT_TORRENT_GET_PRIVATE (torrent);

	priv->files = g_array_new (FALSE, TRUE, sizeof (BtTorrentFile));
	priv->peers = NULL;
	priv->pieces = NULL;

	torrent->io = g_object_new (BT_TYPE_IO, "torrent", torrent, NULL);

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

	g_type_class_add_private (torrent_class, sizeof (BtTorrentPrivate));

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
	pspec = g_param_spec_pointer ("manager",
	                             "torrent manager",
	                             "This torrent's manager",
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
