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

#ifndef __BT_PEER_H__
#define __BT_PEER_H__

#include <glib-object.h>

#include <gnet.h>

#define BT_TYPE_PEER (bt_peer_get_type ())
#define BT_PEER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PEER, BtPeer))
#define BT_IS_PEER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PEER))

typedef struct _BtPeer      BtPeer;
typedef struct _BtPeerClass BtPeerClass;

#include "bt-torrent.h"
#include "bt-manager.h"

typedef enum {
	BT_PEER_STATUS_DISCONNECTED,
	BT_PEER_STATUS_CONNECTED_OUT,
	BT_PEER_STATUS_CONNECTED_IN,
	BT_PEER_STATUS_WAIT_PEER_ID,
	BT_PEER_STATUS_CONNECTED,
	BT_PEER_STATUS_IDLE
} BtPeerStatus;

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
	
	BtPeerStatus status;
};

struct _BtPeerClass {
	GObjectClass parent;
};

GType   bt_peer_get_type ();

BtPeer *bt_peer_new_incoming (BtManager *manager, GTcpSocket *socket);

BtPeer *bt_peer_new_outgoing (BtManager *manager, BtTorrent *torrent, GInetAddr *inetaddr);

void    bt_peer_disconnect (BtPeer *peer);

#endif
