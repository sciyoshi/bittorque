/**
 * bt-peer-protocol.c
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

#include <string.h>

#include "bt-peer-protocol.h"
#include "bt-peer-encryption.h"

typedef enum {
	BT_PEER_DATA_STATUS_NEED_MORE,
	BT_PEER_DATA_STATUS_INVALID,
	BT_PEER_DATA_STATUS_SUCCESS
} BtPeerDataStatus;

static void
bt_peer_write_data (BtPeer *peer, guint len, gpointer buf)
{
	if (peer->encryption_func != NULL) {
		peer->encryption_func (peer, len, buf);
	}
	
	gnet_conn_write (peer->socket, buf, len);
}

void
bt_peer_send_handshake (BtPeer *peer)
{
	gchar *buf;
	gint len;
	
	buf = g_malloc (68);
	
	buf[0] = (gchar) 19;
	
	g_memmove (buf + 1, "BitTorrent protocol", 19);
	
	memset (buf + 20, '\0', 8);
	
	g_memmove (buf + 28, bt_torrent_get_infohash (peer->torrent), 20);
	
	g_memmove (buf + 48, bt_manager_get_peer_id (peer->manager), 20);

	bt_peer_write_data (peer, (guint) len, buf);
}

static BtPeerDataStatus
bt_peer_check_handshake (BtPeer *peer)
{
	if (peer->buffer->len < 1)
		return BT_PEER_DATA_STATUS_NEED_MORE;
	
	if (peer->buffer->str[0] != (gchar) 19)
		return BT_PEER_DATA_STATUS_INVALID;
	
	if (peer->buffer->len < 20)
		return BT_PEER_DATA_STATUS_NEED_MORE;
	
	if (memcmp (peer->buffer->str + 1, "BitTorrent protocol", 19) != 0)
		return BT_PEER_DATA_STATUS_INVALID;
	
	return BT_PEER_DATA_STATUS_SUCCESS;
}

void
bt_peer_data_received (BtPeer *peer, guint len, gpointer buf, gpointer data G_GNUC_UNUSED)
{
	g_return_if_fail (BT_IS_PEER (peer));

	g_string_append_len (peer->buffer, buf, (gssize) len);

	switch (peer->status) {
	case BT_PEER_STATUS_CONNECTED_IN:
		switch (bt_peer_check_handshake (peer)) {
		case BT_PEER_DATA_STATUS_NEED_MORE:
			gnet_conn_read (peer->socket);
			break;

		case BT_PEER_DATA_STATUS_INVALID:
			// TODO: try encryption here
			bt_peer_disconnect (peer);
			break;
		
		default:
			break;
		}
		
		break;
	
	case BT_PEER_STATUS_CONNECTED_OUT:
		break;
	
	default:
		break;
	}
}
