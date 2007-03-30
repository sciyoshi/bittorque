/**
 * bt-utils.h
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

#ifndef __BT_UTILS_H__
#define __BT_UTILS_H__

#include <glib.h>
#include <gnet.h>

#include "bt-bencode.h"
#include "bt-manager.h"

#define BT_ERROR (bt_error_quark ())

/* wrapper type for GTcpSocket */
#define BT_TYPE_TCP_SOCKET (bt_tcp_socket_get_type ())

/* wrapper type for GInetAddr */
#define BT_TYPE_INET_ADDR (bt_inet_addr_get_type ())

typedef enum {
	BT_ERROR_NETWORK,
	BT_ERROR_INVALID_TORRENT,
} BtError;

GQuark   bt_error_quark ();

GType    bt_tcp_socket_get_type ();

GType    bt_inet_addr_get_type ();

gchar   *bt_get_infohash (BtBencode *info);

gchar   *bt_create_peer_id ();

gchar   *bt_url_encode (const gchar *string, gsize size);

gchar   *bt_hash_to_string (const gchar *hash);

gchar   *bt_client_name_from_id (const gchar *id);

gchar   *bt_size_to_string (guint64 size);

#endif
