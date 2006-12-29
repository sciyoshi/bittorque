/**
 * bt-utils.h
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
 * Foundation, Inc., 51 Franklin St,, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __BT_UTILS_H__
#define __BT_UTILS_H__

#include <glib.h>

#include <gnet.h>

#include "bt-bencode.h"
#include "bt-manager.h"

#define BT_TYPE_ERROR (bt_error_get_type ())
#define BT_ERROR (bt_error_get_quark ())

typedef enum {
	BT_ERROR_NETWORK,
	BT_ERROR_RESOLVING,
	BT_ERROR_INVALID_BENCODED_DATA,
	BT_ERROR_INVALID_TORRENT,
	BT_ERROR_PEER_HANDSHAKE,
	BT_ERROR_TRACKER,
	BT_ERROR_INVALID
} BtError;

GType    bt_error_get_type ();

GQuark   bt_error_get_quark ();

gboolean bt_infohash (BtBencode *info, gchar hash[20]);

gboolean bt_create_peer_id (gchar id[21]);

gchar   *bt_url_encode (const gchar *string, gsize size);

gboolean bt_hash_to_string (gchar hash[20], gchar string[41]);

GSource *bt_io_source_create (BtManager *manager, GIOChannel *channel, GIOCondition condition, GSourceFunc callback, gpointer data);

GSource *bt_timeout_source_create (BtManager *manager, guint timeout, GSourceFunc callback, gpointer data);

GSource *bt_idle_source_create (BtManager *manager, GSourceFunc callback, gpointer data);

#endif
