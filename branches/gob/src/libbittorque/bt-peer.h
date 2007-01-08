/**
 * bt-peer.h
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

#ifndef __BT_PEER_H__
#define __BT_PEER_H__

#include <glib.h>
#include <glib-object.h>

#include <gnet.h>

G_BEGIN_DECLS

#define BT_TYPE_PEER        (bt_peer_get_type ())
#define BT_PEER(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PEER, BtPeer))
#define BT_IS_PEER(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PEER))
#define BT_TYPE_PEER_STATUS (bt_peer_status_get_type ())

typedef struct _BtPeer      BtPeer;
typedef struct _BtPeerClass BtPeerClass;

#include "bt-manager.h"
#include "bt-torrent.h"

/* possible status for peer in a state transition type machine */
typedef enum {
	BT_PEER_STATUS_CONNECTING,     /* waiting for a connection */
	BT_PEER_STATUS_CONNECTED_SEND, /* we are connected, and since we initiated, send handshake */
	BT_PEER_STATUS_CONNECTED_WAIT, /* peer connected to us, so wait for his handshake */
	BT_PEER_STATUS_SEND_HANDSHAKE, /* peer has sent his handshake, so send one back */
	BT_PEER_STATUS_WAIT_HANDSHAKE, /* we have sent our handshake, so wait for his */
	BT_PEER_STATUS_DISCONNECTING,  /* trying to disconnect */
	BT_PEER_STATUS_DISCONNECTED,   /* disconnected */
	BT_PEER_STATUS_IDLE_HAVE,      /* handshake is complete, and we should send a HAVE */
	BT_PEER_STATUS_IDLE            /* connected and idle */
} BtPeerStatus;

struct _BtPeer {
	GObject parent;

	BtManager   *manager;
	BtTorrent   *torrent;
	GTcpSocket  *tcp_socket;
	GConn       *socket;
	GInetAddr   *address;
	gchar       *address_string;
	
	gchar       *log_prefix;
	
	gchar       *peer_id;

	GSource     *keepalive_source;

	gboolean    alive;

	BtPeerStatus status;

	GString     *buffer;
	gsize        pos;

	guchar      *bitfield;
	gdouble      completion;

	gboolean     choking;
	gboolean     interesting;
	gboolean     choked;
	gboolean     interested;
};

struct _BtPeerClass {
	GObjectClass parent;
};

GType    bt_peer_get_type ();
GType    bt_peer_status_get_type ();

void     bt_peer_set_choking (BtPeer *self, gboolean choked);
void     bt_peer_set_interesting (BtPeer *self, gboolean choked);

BtPeer  *bt_peer_new (BtManager *manager, BtTorrent *torrent, GTcpSocket *socket, GInetAddr *address);

void     bt_peer_connect (BtPeer *self);
gboolean bt_peer_disconnect (BtPeer *self);

void     bt_peer_send_keepalive (BtPeer *self);
void     bt_peer_send_choke (BtPeer *self);
void     bt_peer_send_unchoke (BtPeer *self);
void     bt_peer_send_interested (BtPeer *self);
void     bt_peer_send_uninterested (BtPeer *self);

void     bt_peer_receive (BtPeer *peer, gchar *buffer, gsize length, gpointer data);

void     bt_peer_send_handshake (BtPeer *self);

G_END_DECLS

#endif
