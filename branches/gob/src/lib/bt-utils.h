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

/* wrapper type for GTcpSocket */
#define BT_TYPE_TCP_SOCKET (bt_tcp_socket_get_type ())

/* wrapper type for GInetAddr */
#define BT_TYPE_INET_ADDR (bt_inet_addr_get_type ())

/* the hexadecimal alphabet if needed */
static const char bt_hex_alphabet[] = "0123456789ABCDEF";

GType    bt_tcp_socket_get_type ();

GType    bt_inet_addr_get_type ();

gchar   *bt_get_info_hash (BtBencode *info);

gchar   *bt_create_peer_id ();

gchar   *bt_url_encode (const gchar *string, gsize size);

gchar   *bt_hash_to_string (const gchar *hash);

gchar   *bt_client_name_from_id (const gchar *id);

GSource *bt_io_source_create (BtManager *manager, GIOChannel *channel, GIOCondition condition, GSourceFunc callback, gpointer data);

GSource *bt_timeout_source_create (BtManager *manager, guint timeout, GSourceFunc callback, gpointer data);

GSource *bt_idle_source_create (BtManager *manager, GSourceFunc callback, gpointer data);

#endif
