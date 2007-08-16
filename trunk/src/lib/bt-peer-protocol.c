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
#include "bt-utils.h"

typedef enum {
	BT_PEER_DATA_STATUS_NEED_MORE,
	BT_PEER_DATA_STATUS_INVALID,
	BT_PEER_DATA_STATUS_SUCCESS
} BtPeerDataStatus;

typedef enum {
	BT_PEER_MSG_CHOKE,
	BT_PEER_MSG_UNCHOKE,
	BT_PEER_MSG_INTERESTED,
	BT_PEER_MSG_UNINTERESTED,
	BT_PEER_MSG_HAVE,
	BT_PEER_MSG_BITFIELD,
	BT_PEER_MSG_REQUEST,
	BT_PEER_MSG_PIECE,
	BT_PEER_MSG_CANCEL
} BtPeerMsg;

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

static guint
bt_peer_get_piece_info (BtPeer* peer, guint block, guint* begin, guint* length)
{
	guint blocks_per_piece = bt_torrent_get_num_blocks (peer->torrent) /
		bt_torrent_get_num_pieces (peer->torrent);
	guint pos_in_piece = block % blocks_per_piece;

	if (begin)
		*begin = pos_in_piece * bt_torrent_get_block_size (peer->torrent);

	if (length)
	*length = bt_torrent_get_block_size (peer->torrent);

	return block / blocks_per_piece;
}

void
bt_peer_send_request (BtPeer *peer, guint block)
{
	gchar buf[17];

	guint len = g_htonl(13);
	g_memmove(buf, &len, 4);

	buf[4] = BT_PEER_MSG_REQUEST;

	guint piece, begin, length;
	piece = g_htonl (bt_peer_get_piece_info (peer, block, &begin, &length));

	g_memmove(buf + 5, &piece, 4);

	begin = g_htonl (begin);
	g_memmove(buf + 9, &begin, 4);

	length = g_htonl (length);
	g_memmove(buf + 13, &length, 4);

	g_debug ("sending request for piece %i, begin %i length %i", g_ntohl (piece), g_ntohl (begin), g_ntohl (length));
	bt_peer_write_data (peer, 17, &buf);
}

static void
bt_peer_send_keep_alive (BtPeer *peer)
{
	// FIXME: check that buf acctually is intialized to all 0
	gchar buf[4];

	bt_peer_write_data (peer, 4, &buf);
}

void
bt_peer_choke (BtPeer *peer)
{
	gchar buf[5];
	guint len = g_htonl(1);

	g_memmove(buf, &len, 4);

	buf[4] = BT_PEER_MSG_CHOKE;

	bt_peer_write_data (peer, 5, &buf);

	peer->choking = TRUE;
}

void
bt_peer_unchoke (BtPeer *peer)
{
	gchar buf[5];
	guint len = g_htonl(1);

	g_memmove(buf, &len, 4);

	buf[4] = BT_PEER_MSG_UNCHOKE;

	bt_peer_write_data (peer, 5, &buf);

	peer->choking = FALSE;
}

void
bt_peer_interest (BtPeer *peer)
{
	gchar buf[5];
	guint len = g_htonl(1);

	g_memmove(buf, &len, 4);

	buf[4] = BT_PEER_MSG_INTERESTED;

	bt_peer_write_data (peer, 5, &buf);

	peer->interested = TRUE;
}

void
bt_peer_uninterest (BtPeer *peer)
{
	gchar buf[5];
	guint len = g_htonl(1);

	g_memmove(buf, &len, 4);

	buf[4] = BT_PEER_MSG_UNINTERESTED;

	bt_peer_write_data (peer, 5, &buf);

	peer->interested = FALSE;
}

static BtPeerDataStatus
bt_peer_check_peer_id (BtPeer *peer)
{
	if (peer->buffer->len < 20)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	// FIXME: make sure we're not already connected to this peer_id
	peer->peer_id = g_memdup (peer->buffer->str, 20);
	
	g_debug ("got peer id, we are connected");
	
	//return BT_PEER_DATA_STATUS_INVALID;
	return BT_PEER_DATA_STATUS_SUCCESS;
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

static BtPeerDataStatus
bt_peer_check_msg_length (BtPeer *peer, guint len, guint expected)
{
	// expected is only used for fixed size messages, otherwise it should be zero
	if (expected && len != expected)
		return BT_PEER_DATA_STATUS_INVALID;

	if (len > peer->buffer->len)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_check_msg (BtPeer *peer)
{
	guint len;
	gint msg;
	guint piece, byte_index, begin, length;
	gchar *peer_name;

	if (peer->buffer->len < 4)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	len = g_ntohl (*((guint *)(peer->buffer->str)));

	g_debug ("got msg length %d buffer len is %" G_GSSIZE_FORMAT, len, peer->buffer->len);
	peer_name = bt_client_name_from_id (peer->peer_id);
	g_debug ("for peer %s", peer_name);
	g_free (peer_name);

	if (len) {
		if (peer->buffer->len < 5)
			return BT_PEER_DATA_STATUS_NEED_MORE;

		msg = peer->buffer->str[4];

		// FIXME: move parsing stuff to bt_peer_check_*
		BtPeerDataStatus len_check;
		switch (msg) {
		case BT_PEER_MSG_CHOKE:
			len_check = bt_peer_check_msg_length (peer, len, 1);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				g_debug ("choked by peer");
				peer->peer_choking = TRUE;
				break;

			default:
				return len_check;
			}
			break;

		case BT_PEER_MSG_UNCHOKE:
			len_check = bt_peer_check_msg_length (peer, len, 1);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				g_debug ("unchoked by peer");
				peer->peer_choking = FALSE;
				break;

			default:
				return len_check;
			}
			break;

		case BT_PEER_MSG_INTERESTED:
			len_check = bt_peer_check_msg_length (peer, len, 1);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				g_debug ("peer interested");
				peer->peer_interested = TRUE;
				break;

			default:
				return len_check;
			}
			break;

		case BT_PEER_MSG_UNINTERESTED:
			len_check = bt_peer_check_msg_length (peer, len, 1);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				g_debug ("peer not interested");
				peer->peer_interested = FALSE;
				break;

			default:
				return len_check;
			}
			break;

		case BT_PEER_MSG_HAVE:
			len_check = bt_peer_check_msg_length (peer, len, 5);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				piece = g_ntohl (*((guint *)(peer->buffer->str + 5)));
				byte_index = piece / 8;
				peer->bitfield[byte_index] |= 1 << ((7 - piece) % 8);
				g_debug ("peer has piece %i", piece);
				break;

			default:
				return len_check;
			}
			break;

		case BT_PEER_MSG_BITFIELD:
			len_check = bt_peer_check_msg_length (peer, len, 0);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				g_debug ("peer sent bitfield");
				peer->bitfield = g_memdup (peer->buffer->str + 5, len - 1);
				// dumping bitfield info for debug
				//guint i = 0;
				//for ( ; i < (len - 1) * 8; i++) {
					//guint byte_index = i / 8;
					//gchar byte = peer->bitfield[byte_index];
					//guint bit = i % 8;
					//g_debug ("bit %i in byte %i is %d", bit, byte_index, (byte >> (7 - bit))&1);
				//}
				break;

			default:
				return len_check;
			}
			break;

		case BT_PEER_MSG_REQUEST:
			len_check = bt_peer_check_msg_length (peer, len, 13);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				piece = g_htonl (*(guint*)(peer->buffer->str + 5));
				begin = g_htonl (*(guint*)(peer->buffer->str + 9));
				length = g_htonl (*(guint*)(peer->buffer->str + 13));
				g_debug ("peer requested piece %i, begin %i, length %i", piece, begin, length);
				break;

			default:
				return len_check;
			}

		case BT_PEER_MSG_PIECE:
			len_check = bt_peer_check_msg_length (peer, len, 0);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				piece = g_htonl (*(guint*)(peer->buffer->str + 5));
				g_debug ("peer sent piece %i", piece);
				break;

			default:
				return len_check;
			}
			break;

		case BT_PEER_MSG_CANCEL:
			len_check = bt_peer_check_msg_length (peer, len, 13);
			switch (len_check) {
			case BT_PEER_DATA_STATUS_SUCCESS:
				piece = g_htonl (*(guint*)(peer->buffer->str + 5));
				begin = g_htonl (*(guint*)(peer->buffer->str + 9));
				length = g_htonl (*(guint*)(peer->buffer->str + 13));
				g_debug ("peer canceled piece %i, begin %i, length %i", piece, begin, length);
				break;

			default:
				return len_check;
			}
			break;

		default:
			return BT_PEER_DATA_STATUS_INVALID;
		}
	}
	else
	{
		// TODO: send keep alive ourselves, not only as response to incomming ones
		g_debug ("peer sent keep-alive");
		bt_peer_send_keep_alive (peer);
	}

	g_string_erase (peer->buffer, 0, len + 4);

	return BT_PEER_DATA_STATUS_SUCCESS;
}

void
bt_peer_data_received (BtPeer *peer, guint len, gpointer buf, gpointer data G_GNUC_UNUSED)
{
	g_return_if_fail (BT_IS_PEER (peer));

	g_debug ("data received for %s", peer->address_string);

	if (buf)
		g_string_append_len (peer->buffer, buf, (gssize) len);

	guint pre_parse_len = peer->buffer->len;

	switch (peer->status) {
	case BT_PEER_STATUS_CONNECTED_IN:
		switch (bt_peer_check_handshake (peer)) {
		case BT_PEER_DATA_STATUS_NEED_MORE:
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
			break;

		case BT_PEER_DATA_STATUS_INVALID:
			// TODO: try encryption here, possibly?
			bt_peer_disconnect (peer);
			break;
		
		default:
			g_debug ("received valid handshake, waiting for peerid");
		
			g_string_erase (peer->buffer, 0, 48);
			peer->status = BT_PEER_STATUS_WAIT_PEER_ID;
			break;
		}
		
		break;

	case BT_PEER_STATUS_WAIT_PEER_ID:
		switch (bt_peer_check_peer_id (peer)) {
		case BT_PEER_DATA_STATUS_NEED_MORE:
			break;

		case BT_PEER_DATA_STATUS_INVALID:
			bt_peer_disconnect (peer);
			break;

		default:
			g_string_erase (peer->buffer, 0, 20);
			peer->status = BT_PEER_STATUS_CONNECTED;
			// for debuging:
			// bt_peer_interest (peer);
			// bt_peer_unchoke (peer);
			break;
		}

		break;

	case BT_PEER_STATUS_CONNECTED:
		switch (bt_peer_check_msg (peer)) {
		case BT_PEER_DATA_STATUS_NEED_MORE:
			break;

		case BT_PEER_DATA_STATUS_INVALID:
			g_debug ("got illegal data from peer");
			bt_peer_disconnect (peer);
			break;

		default:
			// bt_peer_check_msg does the g_string_erase to the correct length
			break;
		}

		break;

	default:
		break;
	}

	// recurse if there is anything left in buffer after we parsed something
	if (peer->buffer->len && peer->buffer->len < pre_parse_len)
		bt_peer_data_received (peer, 0, NULL, NULL);
	else
		gnet_conn_read (peer->socket);
}

