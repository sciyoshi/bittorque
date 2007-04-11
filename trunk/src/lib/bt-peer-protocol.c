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
	gchar buf[68];
	
	buf[0] = (gchar) 19;
	
	g_memmove (buf + 1, "BitTorrent protocol", 19);
	
	memset (buf + 20, '\0', 8);
	
	g_memmove (buf + 28, bt_torrent_get_infohash (peer->torrent), 20);
	
	g_memmove (buf + 48, bt_manager_get_peer_id (peer->manager), 20);

	bt_peer_write_data (peer, (guint) 68, buf);
	
	g_debug ("sent handshake for %s", peer->address_string);
}

static BtPeerDataStatus
bt_peer_check_peer_id (BtPeer *peer)
{
	if (peer->buffer->len < 20)
		return BT_PEER_DATA_STATUS_NEED_MORE;
	
	peer->peer_id = g_memdup (peer->buffer->str, 20);
	
	g_debug ("got peer id, we are connected");
	
	return BT_PEER_DATA_STATUS_INVALID;
}

static BtPeerDataStatus
bt_peer_check_handshake (BtPeer *peer)
{
	gchar infohash[21];

	g_return_val_if_fail (BT_IS_PEER (peer), BT_PEER_DATA_STATUS_INVALID);

	if (peer->buffer->len < 1)
		return BT_PEER_DATA_STATUS_NEED_MORE;
	
	if (peer->buffer->str[0] != (gchar) 19)
		return BT_PEER_DATA_STATUS_INVALID;
	
	if (peer->buffer->len < 20)
		return BT_PEER_DATA_STATUS_NEED_MORE;
	
	if (memcmp (peer->buffer->str + 1, "BitTorrent protocol", 19) != 0)
		return BT_PEER_DATA_STATUS_INVALID;
	
	if (peer->buffer->len < 48)
		return BT_PEER_DATA_STATUS_NEED_MORE;
	
	g_memmove (infohash, peer->buffer->str + 28, 20);
	
	infohash[20] = (gchar) 0;
	
	peer->torrent = bt_manager_get_torrent (peer->manager, infohash);
	
	if (!peer->torrent) {
		g_debug ("connection received for invalid torrent: %s", infohash);
		return BT_PEER_DATA_STATUS_INVALID;
	}
	
	g_debug ("connection received for torrent %s", bt_torrent_get_name (peer->torrent));
	
	g_object_ref (peer->torrent);
	
	bt_torrent_add_peer (peer->torrent, peer);
	
	return BT_PEER_DATA_STATUS_SUCCESS;
}

void
bt_peer_data_received (BtPeer *peer, guint len, gpointer buf, gpointer data G_GNUC_UNUSED)
{
	g_return_if_fail (BT_IS_PEER (peer));

	g_debug ("data received for %s", peer->address_string);

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
			g_string_erase (peer->buffer, 0, 48);
			bt_peer_send_handshake (peer);
			peer->status = BT_PEER_STATUS_WAIT_PEER_ID;
			break;
		}
		
		break;
	
	case BT_PEER_STATUS_CONNECTED_OUT:
		switch (bt_peer_check_handshake (peer)) {
		case BT_PEER_DATA_STATUS_NEED_MORE:
			gnet_conn_read (peer->socket);
			break;

		case BT_PEER_DATA_STATUS_INVALID:
			// TODO: try encryption here, possibly?
			bt_peer_disconnect (peer);
			break;
		
		default:
			g_debug ("received valid handshake, waiting for peerid");
		
			g_string_erase (peer->buffer, 0, 48);
			
			switch (bt_peer_check_peer_id (peer)) {
			case BT_PEER_DATA_STATUS_NEED_MORE:
				gnet_conn_read (peer->socket);
				break;

			case BT_PEER_DATA_STATUS_INVALID:
				bt_peer_disconnect (peer);
				break;
		
			default:
				peer->status = BT_PEER_STATUS_CONNECTED;
				break;
			}

			break;
		}
		
		break;

	case BT_PEER_STATUS_WAIT_PEER_ID:
		
	
	default:
		break;
	}
}
