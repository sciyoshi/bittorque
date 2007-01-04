/**
 * bt-torrent.h
 *
 * Copyright 2006 Samuel Cormier-Iijima <sciyoshi@gmail.com>
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

#ifndef __BT_TORRENT_H__
#define __BT_TORRENT_H__

#include <glib.h>
#include <glib-object.h>

#include <gnet.h>

G_BEGIN_DECLS

#define BT_TYPE_TORRENT    (bt_torrent_get_type ())
#define BT_TORRENT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TORRENT, BtTorrent))
#define BT_IS_TORRENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TORRENT))

#define BT_TYPE_TORRENT_PRIORITY (bt_torrent_priority_get_type ())

#define BT_TYPE_TORRENT_FILE (bt_torrent_file_get_type ())

typedef struct _BtTorrent      BtTorrent;
typedef struct _BtTorrentClass BtTorrentClass;

typedef enum {
	BT_TORRENT_PRIORITY_LOW,
	BT_TORRENT_PRIORITY_NORMAL,
	BT_TORRENT_PRIORITY_HIGH
} BtTorrentPriority;

#include "bt-peer.h"
#include "bt-bencode.h"

typedef struct {
	gchar   *name;
	gint64   size;
	gint64   offset;
	gboolean download;
} BtTorrentFile;

struct _BtTorrent {
	GObject    parent;

	/* manager for this torrent */
	BtManager *manager;

	/* filename of the .torrent file */
	gchar     *filename;
	
	/* location to save data */
	gchar     *location;

	/* torrent name */
	gchar     *name;
	
	/* tracker announce url */
	gchar     *announce;
	
	/* list for multi-tracker torrents */
	GSList    *announce_list;
	
	/* infohash as a 20 byte string */
	gchar     *infohash;
	
	/* infohash as a 40 byte hex string (human readable */
	gchar     *infohash_string;
	
	gchar     *log_domain;
	
	/* total size in bytes of this torrent */
	gint64     size;
	
	/* length of each piece */
	gsize      piece_length;
	
	/* total number of pieces, including last (possibly partially full) one */
	guint      num_pieces;
	
	/* percentage complete */
	gdouble    completion;
	
	guint      num_blocks;
	
	guint      block_size;
	
	gchar     *bitfield;
	
	/* array of BtTorrentFile structures */
	GArray    *files;

	/* socket for tracker connection */
	GConnHttp *tracker_socket;
	guint      tracker_current_tier;
	guint      tracker_current_tracker;
	gint       tracker_interval;
	gint       tracker_min_interval;
	gchar     *tracker_id;

	/* cached pieces */
	GString   *cache;

	/* list of peers for this torrent */
	GSList    *peers;

	GSource   *check_peers_source;
	GSource   *announce_source;
	GTimeVal  *announce_start;
};

struct _BtTorrentClass {
	GObjectClass parent;
};

GType      bt_torrent_priority_get_type ();

GType      bt_torrent_file_get_type ();

GType      bt_torrent_get_type ();

BtTorrent *bt_torrent_new (BtManager *manager, const gchar *filename);

void       bt_torrent_set_location (BtTorrent *torrent, const gchar *location);

gboolean   bt_torrent_start_downloading (BtTorrent *torrent);

gboolean   bt_torrent_announce (BtTorrent *torrent);

gboolean   bt_torrent_announce_stop (BtTorrent *self);

gboolean   bt_torrent_add_peer (BtTorrent *self, BtPeer *peer);

G_END_DECLS

#endif
