/**
 * bt-peer.c
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

#include "bt-peer.h"
#include "bt-peer-protocol.h"
#include "bt-torrent.h"
#include "bt-utils.h"

enum {
	BT_PEER_PROPERTY_TORRENT = 1,
	BT_PEER_PROPERTY_MANAGER,
	BT_PEER_PROPERTY_SOCKET,
	BT_PEER_PROPERTY_ADDRESS
};

enum {
	BT_PEER_SIGNAL_CONNECTED,
	BT_PEER_SIGNAL_HANDSHAKE_COMPLETE,
	BT_PEER_SIGNAL_DATA_RECEIVED,
	BT_PEER_NUM_SIGNALS
};

static guint bt_peer_signals[BT_PEER_NUM_SIGNALS] = {0, };

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
	
	gboolean     peer_choking;
	gboolean     peer_interested;
	gboolean     interested;
	gboolean     choking;
	
	BtPeerStatus status;
};

struct _BtPeerClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (BtPeer, bt_peer, G_TYPE_OBJECT)

BtPeer *
bt_peer_new_incoming (BtManager *manager, GTcpSocket *socket)
{
	return BT_PEER (g_object_new (BT_TYPE_PEER, "manager", manager, "socket", socket, NULL));
}

BtPeer *
bt_peer_new_outgoing (BtManager *manager, BtTorrent *torrent, GInetAddr *address)
{
	return BT_PEER (g_object_new (BT_TYPE_PEER, "manager", manager, "torrent", torrent, "address", address, NULL));
}

static void
bt_peer_connection_callback (GConn *connection G_GNUC_UNUSED, GConnEvent *event, gpointer data)
{
	BtPeer *peer;
	
	g_return_if_fail (BT_IS_PEER (data));
	
	peer = BT_PEER (data);
	
	switch (event->type) {
	case GNET_CONN_CONNECT:
		g_signal_emit (peer, bt_peer_signals[BT_PEER_SIGNAL_CONNECTED], 0);
		break;
	
	case GNET_CONN_CLOSE:
		break;
	
	case GNET_CONN_ERROR:
		break;
	
	case GNET_CONN_READ:
		
		break;
	
	default:
		break;
	}
}

static void
bt_peer_dispose (GObject *object)
{
	BtPeer *self = BT_PEER (object);

	if (self->manager == NULL)
		return;
	
	g_object_unref (self->manager);
	
	self->manager = NULL;
	
	gnet_inetaddr_unref (self->address);

	G_OBJECT_CLASS (bt_peer_parent_class)->dispose (object);
	
	return;
}

static void
bt_peer_finalize (GObject *object)
{
	G_OBJECT_CLASS (bt_peer_parent_class)->finalize (object);
	
	return;
}

static void
bt_peer_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec)
{
	BtPeer *self = BT_PEER (object);

	switch (property) {
	case BT_PEER_PROPERTY_MANAGER:
		self->manager = BT_MANAGER (g_value_dup_object (value));
		break;
		
	case BT_PEER_PROPERTY_ADDRESS:
		self->address = (GInetAddr *) g_value_dup_boxed (value);
		break;
	
	case BT_PEER_PROPERTY_SOCKET:
		self->tcp_socket = (GTcpSocket *) g_value_dup_boxed (value);
		break;

	case BT_PEER_PROPERTY_TORRENT:
		self->torrent = BT_TORRENT (g_value_dup_object (value));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
	
	return;
}

static void
bt_peer_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec)
{
	BtPeer *self = BT_PEER (object);
	
	switch (property) {
	case BT_PEER_PROPERTY_TORRENT:
		self->torrent = BT_TORRENT (g_value_dup_object (value));
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
	
	return;
}

static GObject *
bt_peer_constructor (GType type, guint num, GObjectConstructParam *properties)
{
	GObject *object;
	BtPeer *self;

	gchar *canonical;

	object = G_OBJECT_CLASS (bt_peer_parent_class)->constructor (type, num, properties);
	self = BT_PEER (object);

	if (self->address != NULL) {
		self->socket = gnet_conn_new_inetaddr (self->address, (GConnFunc) bt_peer_connection_callback, self);
		self->status = BT_PEER_STATUS_DISCONNECTED;
	} else {
		self->socket = gnet_conn_new_socket (self->tcp_socket, (GConnFunc) bt_peer_connection_callback, self);
		self->address = gnet_tcp_socket_get_remote_inetaddr (self->tcp_socket);
		self->torrent = NULL;
		self->status = BT_PEER_STATUS_CONNECTED_IN;
		gnet_conn_read (self->socket);
	}

	canonical = gnet_inetaddr_get_canonical_name (self->address);
	self->address_string = g_strdup_printf ("%s:%d", canonical, gnet_inetaddr_get_port (self->address));
	g_free (canonical);

	g_debug ("new connection created with %s", self->address_string);

	gnet_conn_set_watch_error (self->socket, TRUE);

	return object;
}

static void
bt_peer_init (BtPeer *peer)
{
	peer->manager = NULL;
	peer->torrent = NULL;
	peer->socket = NULL;
	peer->address = NULL;

	return;
}

static void
bt_peer_class_init (BtPeerClass *peer_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (peer_class);
	GParamSpec *pspec;
	GClosure *closure;
	GType params[2];
	
	object_class->dispose = bt_peer_dispose;
	object_class->finalize = bt_peer_finalize;
	object_class->constructor = bt_peer_constructor;
	object_class->set_property = bt_peer_set_property;
	object_class->get_property = bt_peer_get_property;

	/**
	 * BtPeer:torrent:
	 *
	 * The #BtTorrent that this peer belongs to.
	 */
	pspec = g_param_spec_object ("torrent",
	                             "the torrent this peer is serving",
	                             "The torrent that this peer is transmitting data for.",
	                             BT_TYPE_TORRENT,
	                             G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK | G_PARAM_CONSTRUCT_ONLY);
	
	g_object_class_install_property (object_class, BT_PEER_PROPERTY_TORRENT, pspec);
	
	/**
	 * BtPeer:manager:
	 *
	 * The #Btmanager that this peer will run under.
	 */
	pspec = g_param_spec_object ("manager",
	                             "the manager for this peer",
	                             "The manager that this peer will run under.",
	                             BT_TYPE_MANAGER,
	                             G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK | G_PARAM_CONSTRUCT_ONLY);
	
	g_object_class_install_property (object_class, BT_PEER_PROPERTY_MANAGER, pspec);

	/**
	 * BtPeer:socket:
	 *
	 * The socket for an incoming connection. Can only be set on construction, and cannot be read.
	 */
	pspec = g_param_spec_boxed ("socket",
	                            "the socket for an incoming connection",
	                            "The socket for an incoming connection. Can only be set on construction, and cannot be read.",
	                            BT_TYPE_TCP_SOCKET,
	                            G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK | G_PARAM_CONSTRUCT_ONLY);
	
	g_object_class_install_property (object_class, BT_PEER_PROPERTY_SOCKET, pspec);

	/**
	 * BtPeer:address:
	 *
	 * The remote address of this peer.
	 */
	pspec = g_param_spec_boxed ("address",
	                            "the address for an outgoing connection",
	                            "The address of this peer.",
	                            BT_TYPE_INET_ADDR,
	                            G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK | G_PARAM_CONSTRUCT_ONLY);
	
	g_object_class_install_property (object_class, BT_PEER_PROPERTY_ADDRESS, pspec);

	/**
	 * BtPeer::connected:
	 *
	 * Emitted when this peer becomes connected.
	 */
	bt_peer_signals[BT_PEER_SIGNAL_CONNECTED] =
		g_signal_newv ("connected",
		               G_TYPE_FROM_CLASS (object_class),
		               G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		               NULL,
		               NULL,
		               NULL,
		               g_cclosure_marshal_VOID__VOID,
		               G_TYPE_NONE,
		               0,
		               NULL);

	/**
	 * BtPeer::handshake-complete:
	 *
	 * Emitted when the bittorrent handshake has been completed.
	 */
	bt_peer_signals[BT_PEER_SIGNAL_HANDSHAKE_COMPLETE] =
		g_signal_newv ("handshake-complete",
		               G_TYPE_FROM_CLASS (object_class),
		               G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		               NULL,
		               NULL,
		               NULL,
		               g_cclosure_marshal_VOID__VOID,
		               G_TYPE_NONE,
		               0,
		               NULL);

	/**
	 * BtPeer::data-received:
	 *
	 * Emitted when data is received from this peer.
	 */
	closure = g_cclosure_new (G_CALLBACK (bt_peer_data_received), NULL, NULL);
	
	params[0] = G_TYPE_UINT;
	params[1] = G_TYPE_POINTER;
	
	bt_peer_signals[BT_PEER_SIGNAL_DATA_RECEIVED] =
		g_signal_newv ("data-received",
		               G_TYPE_FROM_CLASS (object_class),
		               G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		               closure,
		               NULL,
		               NULL,
		               g_cclosure_marshal_VOID__UINT_POINTER,
		               G_TYPE_NONE,
		               2,
		               params);

	return;
}
