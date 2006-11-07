#include <string.h>

#include "bt-peer.h"
#include "bt-utils.h"

G_DEFINE_TYPE (BtPeer, bt_peer, G_TYPE_OBJECT)

static const GEnumValue bt_peer_status_values[] = {
	{BT_PEER_STATUS_CONNECTING, "BT_PEER_STATUS_CONNECTING", "connecting"},
	{BT_PEER_STATUS_CONNECTED_SEND, "BT_PEER_STATUS_CONNECTED_SEND", "connected_send"},
	{BT_PEER_STATUS_CONNECTED_WAIT, "BT_PEER_STATUS_CONNECTED_WAIT", "connected_wait"},
	{BT_PEER_STATUS_SEND_HANDSHAKE, "BT_PEER_STATUS_SEND_HANDSHAKE", "send_handshake"},
	{BT_PEER_STATUS_WAIT_HANDSHAKE, "BT_PEER_STATUS_WAIT_HANDSHAKE", "wait_handshake"},
	{BT_PEER_STATUS_DISCONNECTING, "BT_PEER_STATUS_DISCONNECTING", "disconnecting"},
	{BT_PEER_STATUS_DISCONNECTED, "BT_PEER_STATUS_DISCONNECTED", "disconnected"},
	{BT_PEER_STATUS_IDLE_HAVE, "BT_PEER_STATUS_IDLE_HAVE", "idle_have"},
	{BT_PEER_STATUS_IDLE, "BT_PEER_STATUS_IDLE", "idle"}
};

GType
bt_peer_status_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
		type = g_enum_register_static ("BtPeerStatus", bt_peer_status_values);

	return type;
}

enum {
	PROP_0,
	PROP_CHOKING,
	PROP_INTERESTING,
	PROP_CHOKED,
	PROP_INTERESTED
};

enum {
	SIGNAL_CONNECTED,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

/**
 * bt_peer_set_choking:
 *
 * Sets if we are choking the other peer.
 */

void
bt_peer_set_choking (BtPeer * self, gboolean choking)
{
	g_return_if_fail (BT_IS_PEER (self));
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
	self->interesting = interesting;
}

gboolean
bt_peer_get_choking (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return self->choking;
}

gboolean
bt_peer_get_interesting (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return self->interesting;
}

gboolean
bt_peer_get_choked (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return self->choked;
}

gboolean
bt_peer_get_interested (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return self->interested;
}

gboolean
bt_peer_check (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return FALSE;
}

static void
bt_peer_init_connection (BtPeer *self)
{
	g_return_if_fail (BT_IS_PEER (self));

	
}

static void
bt_peer_connection_callback (GConn *connection G_GNUC_UNUSED, GConnEvent *event, BtPeer *self)
{
	g_return_if_fail (BT_IS_PEER (self));

	switch (event->type) {
	case GNET_CONN_ERROR:
		g_warning ("peer connection error");
		break;

	case GNET_CONN_CONNECT:
		g_debug ("connected to peer");
		self->status = BT_PEER_STATUS_CONNECTED_SEND;
		g_signal_emit (self, signals[SIGNAL_CONNECTED], 0);
		bt_peer_init_connection (self);
		break;

	case GNET_CONN_READ:
		bt_peer_receive (self, event->buffer, event->length);
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

static void
bt_peer_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtPeer *self = BT_PEER (object);

	switch (property) {
	case PROP_CHOKING:
		bt_peer_set_choking (self, g_value_get_boolean (value));

	case PROP_INTERESTING:
		bt_peer_set_interesting (self, g_value_get_boolean (value));
	}
}

static void
bt_peer_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtPeer *self = BT_PEER (object);

	switch (property) {
	case PROP_CHOKING:
		g_value_set_boolean (value, bt_peer_get_choking (self));

	case PROP_INTERESTING:
		g_value_set_boolean (value, bt_peer_get_interesting (self));

	case PROP_CHOKED:
		g_value_set_boolean (value, bt_peer_get_choked (self));

	case PROP_INTERESTED:
		g_value_set_boolean (value, bt_peer_get_interested (self));
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
	peer->buffer = g_string_sized_new (1024);
	peer->status = BT_PEER_STATUS_DISCONNECTED;
}

static GObject *
bt_peer_constructor (GType type, guint num, GObjectConstructParam *properties)
{
	GObject *object;
	BtPeer *self;

	object = G_OBJECT_CLASS (bt_peer_parent_class)->constructor (type, num, properties);
	self = BT_PEER (object);

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

	((GObjectClass *) bt_peer_parent_class)->finalize (peer);
}

static void
bt_peer_class_init (BtPeerClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;

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

	signals[SIGNAL_CONNECTED] =
		g_signal_newv ("connected",
		               BT_TYPE_PEER,
		               G_SIGNAL_RUN_LAST,
		               NULL, NULL, NULL,
		               g_cclosure_marshal_VOID__VOID,
		               G_TYPE_NONE, 0, NULL);
}

/* TODO: put this into a constructor */

BtPeer *
bt_peer_new (BtManager *manager, BtTorrent *torrent, GTcpSocket *socket, GInetAddr *address)
{
	BtPeer *self;

	g_return_val_if_fail (BT_IS_MANAGER (manager), NULL);

	g_return_val_if_fail ((torrent && !socket && address)
			      || (!torrent && socket && !address), NULL);

	self = BT_PEER (g_object_new (BT_TYPE_PEER, NULL));

	self->manager = g_object_ref (manager);

	if (socket) {
		self->socket = gnet_conn_new_socket (socket, (GConnFunc) bt_peer_connection_callback, self);
		self->address = gnet_tcp_socket_get_remote_inetaddr (socket);
		self->status = BT_PEER_STATUS_CONNECTED_WAIT;
		gnet_conn_read (self->socket);
	} else {
		self->address = address;
		self->socket = gnet_conn_new_inetaddr (address, (GConnFunc) bt_peer_connection_callback, self);
		self->torrent = g_object_ref (torrent);
		self->status = BT_PEER_STATUS_DISCONNECTED;
	}

	gnet_conn_set_watch_error (self->socket, TRUE);

	return self;
}