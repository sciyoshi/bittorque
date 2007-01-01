/**
 * bt-peer.c
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

#include <string.h>

#include "bt-peer.h"
#include "bt-utils.h"
#include "bt-marshallers.h"
#include "bt-peer-encryption.h"

G_DEFINE_TYPE (BtPeer, bt_peer, G_TYPE_OBJECT)

static const GEnumValue bt_peer_status_values[] = {
	{BT_PEER_STATUS_CONNECTING,     "BT_PEER_STATUS_CONNECTING",     "connecting"},
	{BT_PEER_STATUS_CONNECTED_SEND, "BT_PEER_STATUS_CONNECTED_SEND", "connected-send"},
	{BT_PEER_STATUS_CONNECTED_WAIT, "BT_PEER_STATUS_CONNECTED_WAIT", "connected-wait"},
	{BT_PEER_STATUS_SEND_HANDSHAKE, "BT_PEER_STATUS_SEND_HANDSHAKE", "send-handshake"},
	{BT_PEER_STATUS_WAIT_HANDSHAKE, "BT_PEER_STATUS_WAIT_HANDSHAKE", "wait-handshake"},
	{BT_PEER_STATUS_DISCONNECTING,  "BT_PEER_STATUS_DISCONNECTING",  "disconnecting"},
	{BT_PEER_STATUS_DISCONNECTED,   "BT_PEER_STATUS_DISCONNECTED",   "disconnected"},
	{BT_PEER_STATUS_IDLE_HAVE,      "BT_PEER_STATUS_IDLE_HAVE",      "idle-have"},
	{BT_PEER_STATUS_IDLE,           "BT_PEER_STATUS_IDLE",           "idle"}
};

enum {
	PROP_0,
	PROP_CHOKING,
	PROP_INTERESTING,
	PROP_CHOKED,
	PROP_INTERESTED,
	PROP_MANAGER,
	PROP_TORRENT,
	PROP_TCP_SOCKET,
	PROP_ADDRESS
};

enum {
	SIGNAL_CONNECTED,
	SIGNAL_DISCONNECTED,
	SIGNAL_DATA_READ,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

GType
bt_peer_status_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
		type = g_enum_register_static ("BtPeerStatus", bt_peer_status_values);

	return type;
}

/**
 * bt_peer_set_choking:
 *
 * Sets if we are choking the other peer.
 */

void
bt_peer_set_choking (BtPeer * self, gboolean choking)
{
	g_return_if_fail (BT_IS_PEER (self));
	
	if (self->choking == choking)
		return;
	
	if (self->choking)
		bt_peer_send_unchoke (self);
	else
		bt_peer_send_choke (self);
	
	self->choking = choking;
}

/**
 * bt_peer_set_interesting:
 *
 * Sets if we are interested in the other peer, i.e. he is "interesting".
 */

void
bt_peer_set_interesting (BtPeer *self, gboolean interesting)
{
	g_return_if_fail (BT_IS_PEER (self));
	
	if (self->interesting == interesting)
		return;
	
	if (self->interesting)
		bt_peer_send_uninterested (self);
	else
		bt_peer_send_interested (self);
	
	self->interesting = interesting;
}

/**
 * bt_peer_get_choking:
 *
 * Returns if we are choking the other peer or not.
 */
 
gboolean
bt_peer_get_choking (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return self->choking;
}

/**
 * bt_peer_get_interesting:
 *
 * Returns if we are interested in the other peer.
 */

gboolean
bt_peer_get_interesting (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return self->interesting;
}

/**
 * bt_peer_get_choked:
 *
 * Returns whether or not the other peer is choking us.
 */

gboolean
bt_peer_get_choked (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return self->choked;
}

/**
 * bt_peer_get_interested:
 *
 * Returns if the peer is interested in us or not.
 */

gboolean
bt_peer_get_interested (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return self->interested;
}

/**
 * bt_peer_check:
 *
 * Check the peers to see who we should optimistically unchoke or choke.
 */

gboolean
bt_peer_check (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	
	/* TODO: add peer checking */
	
	return FALSE;
}

static void
bt_peer_handshake_completed (BtPeer *self, gpointer data G_GNUC_UNUSED)
{
	g_return_if_fail (BT_IS_PEER (self));
	
	
}

/**
 * bt_peer_connected:
 *
 * The default handler for when a peer becomes connected. We read the preferences
 * to see if we should try to connect with encryption or not.
 */

static void
bt_peer_connected (BtPeer *self, gpointer data G_GNUC_UNUSED)
{
	GError *error = NULL;
	BtPeerEncryptionMode encryption;

	g_return_if_fail (BT_IS_PEER (self));

	encryption = (BtPeerEncryptionMode) g_key_file_get_integer (self->manager->preferences, "Protocol", "OutgoingEncryption", &error);

	if (error != NULL || encryption < BT_PEER_ENCRYPTION_MODE_NONE || encryption > BT_PEER_ENCRYPTION_MODE_ONLY) {
		g_warning ("couldn't get preference for encryption, falling back to default: %s", error->message);
		g_clear_error (&error);
		encryption = BT_PEER_ENCRYPTION_MODE_NONE;
	}

	#ifdef BT_ENABLE_ENCRYPTION
	if (encryption == BT_PEER_ENCRYPTION_MODE_NONE) {
		bt_peer_send_handshake (self);
	} else {
		/* TODO: do encryption here */
		bt_peer_send_handshake (self);
		/* bt_peer_encryption_init (self, encryption); */
	}
	#else
	if (encryption != BT_PEER_ENCRYPTION_MODE_NONE) {
		g_warning ("encryption was specified in preferences as enabled, but application was not compiled with encryption support");
	}
	bt_peer_send_handshake (self);
	#endif
	
	gnet_conn_read (self->socket);
}

/**
 * bt_peer_connected:
 *
 * The default handler for when a peer becomes disconnected.
 */

static void
bt_peer_disconnected (BtPeer *self, gpointer data G_GNUC_UNUSED)
{
	if (self->keepalive_source != NULL)
		g_source_destroy (self->keepalive_source);
	
	self->keepalive_source = NULL;
	
	self->alive = FALSE;
}

/**
 * bt_peer_connection_callback:
 *
 * The callback for all peer events; for when data has been read or written, or the peer has
 * become connected.
 */

static void
bt_peer_connection_callback (GConn *connection G_GNUC_UNUSED, GConnEvent *event, BtPeer *self)
{
	g_return_if_fail (BT_IS_PEER (self));

	switch (event->type) {
	case GNET_CONN_ERROR:
		g_debug ("peer connection error for %s", self->address_string);
		break;

	case GNET_CONN_CLOSE:
		g_debug ("closing connection for %s", self->address_string);
		g_signal_emit (self, signals[SIGNAL_DISCONNECTED], 0);
		break;

	case GNET_CONN_CONNECT:
		/* TODO: free string */
		g_debug ("connected to peer %s", self->address_string);
		self->status = BT_PEER_STATUS_CONNECTED_SEND;
		self->alive = TRUE;
		g_signal_emit (self, signals[SIGNAL_CONNECTED], 0);
		break;

	case GNET_CONN_READ:
		g_signal_emit (self, signals[SIGNAL_DATA_READ], 0, event->buffer, event->length);
		break;

	case GNET_CONN_WRITE:
		break;

	default:
		break;
	}

	return;
}

void
bt_peer_connect (BtPeer *self)
{
	self->status = BT_PEER_STATUS_CONNECTING;
	gnet_conn_connect (self->socket);
}

gboolean
bt_peer_disconnect (BtPeer *self)
{
	self->status = BT_PEER_STATUS_DISCONNECTED;
	gnet_conn_disconnect (self->socket);
	return FALSE;
}

static void
bt_peer_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec)
{
	BtPeer *self = BT_PEER (object);

	switch (property) {
	case PROP_CHOKING:
		bt_peer_set_choking (self, g_value_get_boolean (value));
		break;

	case PROP_INTERESTING:
		bt_peer_set_interesting (self, g_value_get_boolean (value));
		break;
	
	case PROP_MANAGER:
		self->manager = BT_MANAGER (g_value_dup_object (value));
		break;
	
	case PROP_TORRENT:
		self->torrent = BT_TORRENT (g_value_dup_object (value));
		break;
	
	case PROP_TCP_SOCKET:
		self->tcp_socket = (GTcpSocket *) g_value_dup_boxed (value);
		break;
	
	case PROP_ADDRESS:
		self->address = (GInetAddr *) g_value_dup_boxed (value);
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
		break;
	}
}

static void
bt_peer_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec)
{
	BtPeer *self = BT_PEER (object);

	switch (property) {
	case PROP_CHOKING:
		g_value_set_boolean (value, bt_peer_get_choking (self));
		break;

	case PROP_INTERESTING:
		g_value_set_boolean (value, bt_peer_get_interesting (self));
		break;

	case PROP_CHOKED:
		g_value_set_boolean (value, bt_peer_get_choked (self));
		break;

	case PROP_INTERESTED:
		g_value_set_boolean (value, bt_peer_get_interested (self));
		break;
	
	case PROP_MANAGER:
		g_value_set_object (value, self->manager);
		break;
	
	case PROP_ADDRESS:
		g_value_set_boxed (value, self->address);
		break;

	case PROP_TCP_SOCKET:
		g_value_set_boxed (value, self->tcp_socket);
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
		break;
	}
}

static void
bt_peer_init (BtPeer *peer)
{
	peer->manager = NULL;
	peer->torrent = NULL;
	peer->address = NULL;
	peer->socket = NULL;
	peer->interested = FALSE;
	peer->choked = TRUE;
	peer->interesting = FALSE;
	peer->choking = TRUE;
	peer->pos = 0;
	peer->log_prefix = NULL;
	peer->buffer = g_string_sized_new (1024);
	peer->status = BT_PEER_STATUS_DISCONNECTED;
	peer->peer_id = NULL;
	peer->keepalive_source = NULL;
	peer->alive = FALSE;
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
		self->tcp_socket = self->socket->socket;
		self->status = BT_PEER_STATUS_DISCONNECTED;
	} else {
		self->socket = gnet_conn_new_socket (self->tcp_socket, (GConnFunc) bt_peer_connection_callback, self);
		self->address = gnet_tcp_socket_get_remote_inetaddr (self->tcp_socket);
		self->torrent = NULL;
		self->status = BT_PEER_STATUS_CONNECTED_WAIT;
		gnet_conn_read (self->socket);
	}

	canonical = gnet_inetaddr_get_canonical_name (self->address);
	self->address_string = g_strdup_printf ("%s:%d", canonical, gnet_inetaddr_get_port (self->address));
	g_free (canonical);

	gnet_conn_set_watch_error (self->socket, TRUE);
	
	return object;
}

static void
bt_peer_dispose (GObject *peer)
{
	BtPeer *self = BT_PEER (peer);

	if (self->manager == NULL)
		return;

	g_object_unref (self->manager);
	self->manager = NULL;

	if (self->torrent) {
		g_object_unref (self->torrent);
		self->torrent = NULL;
	}

	((GObjectClass *) bt_peer_parent_class)->finalize (peer);
}

static void
bt_peer_finalize (GObject *peer)
{
	BtPeer *self = BT_PEER (peer);

	if (self->socket) {
		gnet_conn_delete (self->socket);
		self->socket = NULL;
	}

	if (self->address) {
		gnet_inetaddr_delete (self->address);
		self->address = NULL;
	}

	g_string_free (self->buffer, TRUE);
	g_free (self->log_prefix);

	((GObjectClass *) bt_peer_parent_class)->finalize (peer);
}

static void
bt_peer_class_init (BtPeerClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;
	GClosure *closure;
	GType params[2];

	gclass = G_OBJECT_CLASS (klass);

	gclass->set_property = bt_peer_set_property;
	gclass->get_property = bt_peer_get_property;
	gclass->finalize = bt_peer_finalize;
	gclass->dispose = bt_peer_dispose;
	gclass->constructor = bt_peer_constructor;

	pspec = g_param_spec_boolean ("choking",
	                              "choking peer",
	                              "TRUE if we are choking the peer",
	                              TRUE,
	                              G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_CHOKING, pspec);

	pspec = g_param_spec_boolean ("interesting",
	                              "interested in peer",
	                              "TRUE if we are interested in the peer, i.e. he is \"interesting\"",
	                              FALSE,
	                              G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_INTERESTING, pspec);

	pspec = g_param_spec_boolean ("choked",
	                              "choked by peer",
	                              "TRUE if we are being choked by the peer",
	                              TRUE,
	                              G_PARAM_READABLE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_CHOKED, pspec);

	pspec = g_param_spec_boolean ("interested",
	                              "peer is interested in us",
	                              "TRUE if the peer is interested in downloading from us",
	                              FALSE,
	                              G_PARAM_READABLE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_INTERESTED, pspec);
	
	pspec = g_param_spec_object ("manager",
	                             "torrent manager",
	                             "The manager for this torrent",
	                             BT_TYPE_MANAGER,
	                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_MANAGER, pspec);
	
	pspec = g_param_spec_object ("torrent",
	                             "torrent this peer is for",
	                             "The torrent that this peer is for",
	                             BT_TYPE_TORRENT,
	                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_TORRENT, pspec);
	
	pspec = g_param_spec_boxed ("tcp-socket",
	                            "the TCP socket that this peer connected to",
	                            "A GTCPSocket that is connected to this peer",
	                            BT_TYPE_TCP_SOCKET,
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_TCP_SOCKET, pspec);
	
	pspec = g_param_spec_boxed ("address",
	                            "the GInetAddr of the remote peer",
	                            "The address and port of the remote peer",
	                            BT_TYPE_INET_ADDR,
	                            G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_ADDRESS, pspec);

	closure = g_cclosure_new (G_CALLBACK (bt_peer_connected), NULL, NULL);

	signals[SIGNAL_CONNECTED] =
		g_signal_newv ("connected",
		               BT_TYPE_PEER,
		               G_SIGNAL_RUN_LAST,
		               closure, NULL, NULL,
		               g_cclosure_marshal_VOID__VOID,
		               G_TYPE_NONE, 0, NULL);

	closure = g_cclosure_new (G_CALLBACK (bt_peer_disconnected), NULL, NULL);

	signals[SIGNAL_DISCONNECTED] =
		g_signal_newv ("disconnected",
		               BT_TYPE_PEER,
		               G_SIGNAL_RUN_LAST,
		               closure, NULL, NULL,
		               g_cclosure_marshal_VOID__VOID,
		               G_TYPE_NONE, 0, NULL);
	
	closure = g_cclosure_new (G_CALLBACK (bt_peer_handshake_completed), NULL, NULL);

	signals[SIGNAL_DISCONNECTED] =
		g_signal_newv ("handshake-completed",
		               BT_TYPE_PEER,
		               G_SIGNAL_RUN_LAST,
		               closure, NULL, NULL,
		               g_cclosure_marshal_VOID__VOID,
		               G_TYPE_NONE, 0, NULL);

	params[0] = G_TYPE_POINTER;
	params[1] = G_TYPE_UINT;

	closure = g_cclosure_new (G_CALLBACK (bt_peer_receive), NULL, NULL);

	signals[SIGNAL_DATA_READ] =
		g_signal_newv ("data-read",
		               BT_TYPE_PEER,
		               G_SIGNAL_RUN_LAST,
		               closure, NULL, NULL,
		               bt_cclosure_marshal_VOID__POINTER_UINT,
		               G_TYPE_NONE, 2, params);
}

/* FIXME: put this into a constructor */

BtPeer *
bt_peer_new (BtManager *manager, BtTorrent *torrent, GTcpSocket *socket, GInetAddr *address)
{
	BtPeer *self;
	
	g_return_val_if_fail (BT_IS_MANAGER (manager), NULL);

	g_return_val_if_fail ((torrent && !socket && address) || (!torrent && socket && !address), NULL);

	if (socket == NULL)
		self = BT_PEER (g_object_new (BT_TYPE_PEER, "manager", manager, "torrent", torrent, "address", address, NULL));
	else
		self = BT_PEER (g_object_new (BT_TYPE_PEER, "manager", manager, "tcp-socket", socket, NULL));

	return self;
}
