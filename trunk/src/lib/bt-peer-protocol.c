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

#include "bt-peer-private.h"
#include "bt-peer-protocol.h"
#include "bt-peer-encryption.h"
#include "bt-utils.h"

// defines for fixed length messages, the 4-byte length prefix included
// bitfield and piece messages are of variable lengths
#define BT_PEER_MSG_LENGTH_CHOKE 5
#define BT_PEER_MSG_LENGTH_UNCHOKE 5
#define BT_PEER_MSG_LENGTH_INTERESTED 5
#define BT_PEER_MSG_LENGTH_UNINTERESTED 5
#define BT_PEER_MSG_LENGTH_HAVE 9
#define BT_PEER_MSG_LENGTH_REQUEST 17
#define BT_PEER_MSG_LENGTH_CANCEL 17
#define BT_PEER_MSG_LENGTH_DHT_PORT 7
#define BT_PEER_MSG_LENGTH_KEEP_ALIVE 4

typedef enum {
	BT_PEER_MSG_CHOKE,
	BT_PEER_MSG_UNCHOKE,
	BT_PEER_MSG_INTERESTED,
	BT_PEER_MSG_UNINTERESTED,
	BT_PEER_MSG_HAVE,
	BT_PEER_MSG_BITFIELD,
	BT_PEER_MSG_REQUEST,
	BT_PEER_MSG_PIECE,
	BT_PEER_MSG_CANCEL,
	BT_PEER_MSG_DHT_PORT,
	BT_PEER_MSG_KEEP_ALIVE // helper, not really a protocol message
} BtPeerMsg;

/* declare handlers for lookup table */
static BtPeerDataStatus bt_peer_on_choke (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_unchoke (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_interested (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_uninterested (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_have (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_bitfield (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_request (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_piece (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_cancel (BtPeer *peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_dht_port (BtPeer* peer, guint *bytes_read);
static BtPeerDataStatus bt_peer_on_keep_alive (BtPeer *peer, guint *bytes_read);

static BtPeerMsgFunc handler_lookup_table[] = {
	&bt_peer_on_choke,
	&bt_peer_on_unchoke,
	&bt_peer_on_interested,
	&bt_peer_on_uninterested,
	&bt_peer_on_have,
	&bt_peer_on_bitfield,
	&bt_peer_on_request,
	&bt_peer_on_piece,
	&bt_peer_on_cancel,
	&bt_peer_on_dht_port,
	NULL, // no global handler for extensions
	&bt_peer_on_keep_alive
};

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

static void G_GNUC_UNUSED
bt_peer_send_keep_alive (BtPeer *peer)
{
	gchar buf[4] = {0, 0, 0, 0};

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
bt_peer_on_choke (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;

	// redundant, checked in bt_peer_peek_msg_type
	if (peer->buffer->len < 5)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_CHOKE, BT_PEER_DATA_STATUS_INVALID);

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	// this message is fixed length
	if (msg_len != 1)
		return BT_PEER_DATA_STATUS_INVALID;

	peer->peer_choking = TRUE;
	g_debug ("choked by peer");

	*bytes_read = 5;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_unchoke (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;

	// redundant, checked in bt_peer_peek_msg_type
	if (peer->buffer->len < 5)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_UNCHOKE, BT_PEER_DATA_STATUS_INVALID);

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	// this message is fixed length
	if (msg_len != 1)
		return BT_PEER_DATA_STATUS_INVALID;

	peer->peer_choking = FALSE;
	g_debug ("unchoked by peer");

	*bytes_read = 5;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_interested (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;

	// redundant, checked in bt_peer_peek_msg_type
	if (peer->buffer->len < 5)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_INTERESTED, BT_PEER_DATA_STATUS_INVALID);

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	// this message is fixed length
	if (msg_len != 1)
		return BT_PEER_DATA_STATUS_INVALID;

	peer->peer_interested = TRUE;
	g_debug ("peer interested");

	*bytes_read = 5;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_uninterested (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;

	// redundant, checked in bt_peer_peek_msg_type
	if (peer->buffer->len < 5)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_UNINTERESTED, BT_PEER_DATA_STATUS_INVALID);

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	// this message is fixed length
	if (msg_len != 1)
		return BT_PEER_DATA_STATUS_INVALID;

	peer->peer_interested = FALSE;
	g_debug ("peer uninterested");

	*bytes_read = 5;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_have (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;
	guint32 piece;

	if (peer->buffer->len < 9)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_HAVE, BT_PEER_DATA_STATUS_INVALID);

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	// this message is fixed length
	if (msg_len != 5)
		return BT_PEER_DATA_STATUS_INVALID;

	piece = g_ntohl (*((guint32*)(peer->buffer->str + 5)));
	peer->bitfield[piece / 8] |= 1 << (7 - (piece % 8));
	g_debug ("peer has piece %i", piece);

	*bytes_read = 9;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_bitfield (BtPeer* peer, guint* bytes_read)
{
	guint32 msg_len;

	// redundant, checked in bt_peer_peek_msg_type
	if (peer->buffer->len < 5)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_BITFIELD, BT_PEER_DATA_STATUS_INVALID);

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	if ((msg_len - 1) * 8 < bt_torrent_get_num_pieces (peer->torrent))
		return BT_PEER_DATA_STATUS_INVALID;

	if (msg_len > peer->buffer->len)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	peer->bitfield = g_memdup (peer->buffer->str + 5, msg_len - 1);
	g_debug ("peer sent bitfield");

	*bytes_read = msg_len + 4;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_request (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;
	guint32 piece, begin, length;

	if (peer->buffer->len < 17)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_REQUEST, BT_PEER_DATA_STATUS_INVALID);

	// this message is fixed length
	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));
	if (msg_len != 13)
		return BT_PEER_DATA_STATUS_INVALID;

	piece = g_htonl (*(guint32*)(peer->buffer->str + 5));
	begin = g_htonl (*(guint32*)(peer->buffer->str + 9));
	length = g_htonl (*(guint32*)(peer->buffer->str + 13));
	g_debug ("peer requested piece %i, begin %i, length %i", piece, begin, length);

	// mainline disconnects requests greater than 2^17, so do we
	if (length > 131072)
		return BT_PEER_DATA_STATUS_INVALID;

	*bytes_read = 17;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_piece (BtPeer* peer, guint* bytes_read)
{
	guint32 msg_len;
	guint32 piece, begin;
	// gchar* data;

	// redundant, checked in bt_peer_peek_msg_type
	if (peer->buffer->len < 5)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_PIECE, BT_PEER_DATA_STATUS_INVALID);

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	// incoming data should be _at least_ 1 byte
	if (msg_len <= 13)
		return BT_PEER_DATA_STATUS_INVALID;

	if (msg_len - 1 > bt_torrent_get_piece_length (peer->torrent))
		return BT_PEER_DATA_STATUS_INVALID;

	if (msg_len > peer->buffer->len)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	// FIXME: make sure we previously requested the incoming block

	piece = g_htonl (*(guint*)(peer->buffer->str + 5));
	begin = g_htonl (*(guint*)(peer->buffer->str + 9));
	// data = g_memdup (peer->buffer->str + 13, msg_len - 1);
	g_debug ("peer sent piece %i beginning at %i", piece, begin);

	*bytes_read = msg_len + 4;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_cancel (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;
	guint32 piece, begin, length;

	if (peer->buffer->len < 17)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_CANCEL, BT_PEER_DATA_STATUS_INVALID);

	// this message is fixed length
	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));
	if (msg_len != 13)
		return BT_PEER_DATA_STATUS_INVALID;

	piece = g_htonl (*(guint32*)(peer->buffer->str + 5));
	begin = g_htonl (*(guint32*)(peer->buffer->str + 9));
	length = g_htonl (*(guint32*)(peer->buffer->str + 13));
	g_debug ("peer canceled piece %i, begin %i, length %i", piece, begin, length);

	*bytes_read = 17;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_dht_port (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;
	guint16 port;

	if (peer->buffer->len < 7)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	g_return_val_if_fail (peer->buffer->str[4] == BT_PEER_MSG_DHT_PORT, BT_PEER_DATA_STATUS_INVALID);

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	// this message is fixed length
	if (msg_len != 3)
		return BT_PEER_DATA_STATUS_INVALID;

	port = g_ntohl (*((guint16*)(peer->buffer->str + 5)));

	g_debug ("peer sent dht port %i", port);

	*bytes_read = 7;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_on_keep_alive (BtPeer* peer, guint *bytes_read)
{
	guint32 msg_len;

	if (peer->buffer->len < 4)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));

	if (msg_len != 0)
		return BT_PEER_DATA_STATUS_INVALID;

	bt_peer_send_keep_alive (peer);

	*bytes_read = 4;

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_peek_msg_type (BtPeer* peer, BtPeerMsg* type)
{
	guint32 msg_len;

	if (peer->buffer->len < 4)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	msg_len = g_ntohl (*((guint32*)(peer->buffer->str)));
	if (msg_len == 0)
	{
		*type = BT_PEER_MSG_KEEP_ALIVE;
		return BT_PEER_DATA_STATUS_SUCCESS;
	}

	if (peer->buffer->len < 5)
		return BT_PEER_DATA_STATUS_NEED_MORE;

	// only 0 through 8 are valid protocol messages, 9 is a mainline extension
	if (peer->buffer->str[4] > 9)
		return BT_PEER_DATA_STATUS_INVALID;

	*type = (BtPeerMsg) peer->buffer->str[4];

	return BT_PEER_DATA_STATUS_SUCCESS;
}

static BtPeerDataStatus
bt_peer_handle_msg (BtPeer* peer, guint* bytes_read)
{
	BtPeerDataStatus status;
	BtPeerMsg type;
	BtPeerMsgFunc handler = NULL;

	status = bt_peer_peek_msg_type (peer, &type);

	switch (status)
	{
	case BT_PEER_DATA_STATUS_SUCCESS:
		handler = handler_lookup_table[type];
		break;

	case BT_PEER_DATA_STATUS_INVALID:
		// try extension handler if we have one
		if (peer->extension_func != NULL)
			return peer->extension_func (peer, bytes_read);
		break;

	default:
		return status;
		
	}

	g_return_val_if_fail (handler != NULL, BT_PEER_DATA_STATUS_INVALID);

	return handler (peer, bytes_read);
}

void
bt_peer_data_received (BtPeer *peer, guint len, gpointer buf, gpointer data G_GNUC_UNUSED)
{
	guint bytes_read = 0;

	g_return_if_fail (BT_IS_PEER (peer));

	g_debug ("data received for %s", peer->address_string);

	if (buf)
		g_string_append_len (peer->buffer, buf, (gssize) len);

	switch (peer->status) {
	case BT_PEER_STATUS_CONNECTED_IN:
		switch (bt_peer_check_handshake (peer)) {
		case BT_PEER_DATA_STATUS_NEED_MORE:
			gnet_conn_read (peer->socket);
			return;

		case BT_PEER_DATA_STATUS_INVALID:
			// TODO: try encryption here
			bt_peer_disconnect (peer);
			return;
		
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
			return;

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
			gnet_conn_read (peer->socket);
			return;

		case BT_PEER_DATA_STATUS_INVALID:
			bt_peer_disconnect (peer);
			return;

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
		switch (bt_peer_handle_msg (peer, &bytes_read))
		{
		case BT_PEER_DATA_STATUS_NEED_MORE:
			gnet_conn_read (peer->socket);
			return;

		case BT_PEER_DATA_STATUS_INVALID:
			g_debug ("got illegal data from peer");
			bt_peer_disconnect (peer);
			return;

		default:
			g_string_erase (peer->buffer, 0, bytes_read);
			break;
		}

		break;

	default:
		break;
	}

	
	// recurse if there is anything left in buffer
	if (peer->buffer->len)
		bt_peer_data_received (peer, 0, NULL, NULL);
	else
		gnet_conn_read (peer->socket);
}

