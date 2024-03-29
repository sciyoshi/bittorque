/**
 * bt-peer.h
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

#ifndef __BT_PEER_PRIVATE_H__
#define __BT_PEER_PRIVATE_H__

#include <gnet.h>

#include "bt-peer.h"
 
typedef enum {
	BT_PEER_STATUS_DISCONNECTED,
	BT_PEER_STATUS_CONNECTED_OUT,
	BT_PEER_STATUS_CONNECTED_IN,
	BT_PEER_STATUS_WAIT_PEER_ID,
	BT_PEER_STATUS_CONNECTED,
	BT_PEER_STATUS_IDLE
} BtPeerStatus;

typedef enum {
	BT_PEER_DATA_STATUS_NEED_MORE,
	BT_PEER_DATA_STATUS_INVALID,
	BT_PEER_DATA_STATUS_SUCCESS
} BtPeerDataStatus;

typedef BtPeerDataStatus (*BtPeerMsgFunc) (BtPeer *peer, guint* bytes_read);

struct _BtPeer {
	GObject      parent;
	
	/* the BtManager for this peer */
	BtManager   *manager;
	
	/* the torrent that this peer is serving */
	BtTorrent   *torrent;
	
	/* the internet address of this peer */
	GInetAddr   *address;
	
	/* the address as a string address:port */
	gchar       *address_string;
	
	/* the network connection */
	GConn       *socket;
	
	/* the actual tcp socket */
	GTcpSocket  *tcp_socket;

	gchar       *peer_id;
	
	gboolean     peer_choking;
	gboolean     peer_interested;
	gboolean     interested;
	gboolean     choking;

	gchar       *bitfield;

	GString     *buffer;
	
	void       (*encryption_func) (BtPeer *peer, guint len, gpointer buf);

	BtPeerMsgFunc extension_func;

	BtPeerStatus status;
};

struct _BtPeerClass {
	GObjectClass parent;
};

#endif
