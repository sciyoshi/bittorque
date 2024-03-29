/**
 * bt-torrent.h
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

#ifndef __BT_TORRENT_H__
#define __BT_TORRENT_H__

#include <glib-object.h>

#define BT_TYPE_TORRENT (bt_torrent_get_type ())
#define BT_TORRENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_TORRENT, BtTorrent))
#define BT_IS_TORRENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_TORRENT))

typedef struct _BtTorrent        BtTorrent;
typedef struct _BtTorrentClass   BtTorrentClass;

#include "bt-manager.h"
#include "bt-peer.h"
#include "bt-torrent-file.h"
#include "bt-io.h"

struct _BtTorrent {
	GObject    parent;

	/* the io object for this torrent */
	BtIO *io;
};

struct _BtTorrentClass {
	GObjectClass parent;
};

GType      bt_torrent_get_type ();

BtTorrent *bt_torrent_new (BtManager *manager, gchar *filename, GError **error);

const gchar          *bt_torrent_get_infohash (BtTorrent *torrent);

const gchar          *bt_torrent_get_infohash_string (BtTorrent *torrent);

const gchar          *bt_torrent_get_name (BtTorrent *torrent);

guint64               bt_torrent_get_size (BtTorrent *torrent);

guint                 bt_torrent_get_num_pieces (BtTorrent *torrent);

guint32               bt_torrent_get_piece_length (BtTorrent *torrent);

guint32               bt_torrent_get_piece_length_extended (BtTorrent *torrent, guint piece);

const gchar          *bt_torrent_get_piece_hash (BtTorrent* torrent, guint piece);

guint                 bt_torrent_get_num_blocks (BtTorrent *torrent);

guint                 bt_torrent_get_block_size (BtTorrent *torrent);

guint                 bt_torrent_get_block_size_extended (BtTorrent *torrent, guint block);

const BtTorrentFile  *bt_torrent_get_file (BtTorrent *torrent, guint index);

guint                 bt_torrent_get_num_files (BtTorrent *torrent);

void                  bt_torrent_start (BtTorrent *torrent);

void                  bt_torrent_stop (BtTorrent *torrent);

void                  bt_torrent_pause (BtTorrent *torrent);

void                  bt_torrent_add_peer (BtTorrent *torrent, BtPeer *peer);

void                  bt_torrent_tracker_announce (BtTorrent *torrent);

void                  bt_torrent_tracker_stop_announce (BtTorrent *torrent);

#endif
