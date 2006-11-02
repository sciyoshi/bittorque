#include <string.h>

#include "bt-peer.h"
#include "bt-utils.h"

G_DEFINE_TYPE (BtPeer, bt_peer, G_TYPE_OBJECT)

static const GEnumValue bt_peer_status_values[] = {
	{BT_PEER_STATUS_CONNECTING, "BT_PEER_STATUS_CONNECTING", "connecting"},
	{BT_PEER_STATUS_CONNECTED, "BT_PEER_STATUS_CONNECTED", "connected"},
	{BT_PEER_STATUS_HANDSHAKE, "BT_PEER_STATUS_HANDSHAKE", "handshake"},
	{BT_PEER_STATUS_HANDSHAKE_PEER_ID, "BT_PEER_STATUS_HANDSHAKE_PEER_ID", "handshake_peer_id"},
	{BT_PEER_STATUS_DISCONNECTING, "BT_PEER_STATUS_DISCONNECTING", "disconnecting"},
	{BT_PEER_STATUS_DISCONNECTED, "BT_PEER_STATUS_DISCONNECTED", "disconnected"},
	{BT_PEER_STATUS_IDLE_HAVE, "BT_PEER_STATUS_IDLE_HAVE", "idle_have"},
	{BT_PEER_STATUS_IDLE, "BT_PEER_STATUS_IDLE", "idle"}
};

static const GEnumValue bt_peer_send_status_values[] = {
	{BT_PEER_SEND_STATUS_HANDSHAKE, "BT_PEER_SEND_STATUS_HANDSHAKE", "handshake"},
	{BT_PEER_SEND_STATUS_HANDSHAKING, "BT_PEER_SEND_STATUS_HANDSHAKING", "handshaking"},
	{BT_PEER_SEND_STATUS_DISCONNECTED, "BT_PEER_SEND_STATUS_DISCONNECTED", "disconnected"},
	{BT_PEER_SEND_STATUS_IDLE, "BT_PEER_SEND_STATUS_IDLE", "idle"}
};

GType
bt_peer_status_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
		type = g_enum_register_static ("BtPeerStatus", bt_peer_status_values);

	return type;
}


GType
bt_peer_send_status_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
		type = g_enum_register_static ("BtPeerSendStatus", bt_peer_send_status_values);

	return type;
}

enum {
	PROP_0,
	PROP_CHOKING,
	PROP_INTERESTING,
	PROP_CHOKED,
	PROP_INTERESTED
};

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
	peer->send_status = BT_PEER_STATUS_DISCONNECTED;
}

static void
bt_peer_class_init (BtPeerClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;

	gclass = G_OBJECT_CLASS (klass);

	gclass->set_property = bt_peer_set_property;
	gclass->get_property = bt_peer_get_property;

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
}

/**
 * bt_peer_set_choking:
 *
 * Sets if we are choking the other peer.
 */

void
bt_peer_set_choking (BtPeer *self, gboolean choking)
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
	g_return_val_if_fail (BT_IS_PEER (self), -1);
	return self->choking;
}

gboolean
bt_peer_get_interesting (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), -1);
	return self->interesting;
}

gboolean
bt_peer_get_choked (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), -1);
	return self->choked;
}

gboolean
bt_peer_get_interested (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), -1);
	return self->interested;
}


gboolean
bt_peer_check (BtPeer *self)
{
	g_return_val_if_fail (BT_IS_PEER (self), FALSE);
	return FALSE;
}

static gboolean
bt_peer_send_handshake (BtPeer *self)
{
	GError *error;
	gchar buf[69];

	error = NULL;

	g_return_val_if_fail (self->send_status == BT_PEER_SEND_STATUS_HANDSHAKE, FALSE);
	g_return_val_if_fail (self->torrent != NULL, FALSE);

	buf[0] = 19;
	strncpy (buf + 1, "BitTorrent protocol", 19);
	memset (buf + 20, '\0', 8);
	strncpy (buf + 28, self->torrent->infohash, 20);
	strncpy (buf + 48, self->manager->peer_id, 20);

	g_debug ("sending handshake");

	self->send_status = BT_PEER_SEND_STATUS_HANDSHAKING;

	gnet_conn_write (self->socket, buf, 68);

	return TRUE;
}

static void
bt_peer_receive (BtPeer *self, gchar *buf, gsize len)
{
	gchar *infohash;

	g_return_if_fail (BT_IS_PEER (self));

	if (len == 0) {
		g_debug ("connection closed");
		goto cleanup;
	}

	g_string_append_len (self->buffer, buf, len);

	self->pos += len;

	if (self->status == BT_PEER_STATUS_HANDSHAKE) {
		if (self->pos < 48)
			goto wait;

		if (self->buffer->str[0] != 19) {
			g_warning ("bad header sent");
			goto cleanup;
		}

		if (strncmp (self->buffer->str + 1, "BitTorrent protocol", 19) != 0) {
			g_warning ("bad header sent");
			goto cleanup;
		}

		infohash = g_strndup (self->buffer->str + 28, 20);
		self->torrent = bt_manager_get_torrent (self->manager, infohash);
		if (!self->torrent) {
			g_warning ("connection received for torrent we are not serving");
			g_free (infohash);
			goto cleanup;
		}

		g_free (infohash);
		g_object_ref (self->torrent);

		g_string_erase (self->buffer, 0, 48);

		self->pos -= 48;

		g_debug ("successfuly handshaked");

		if (self->send_status == BT_PEER_SEND_STATUS_IDLE) {

		} else {
			self->send_status = BT_PEER_SEND_STATUS_HANDSHAKE;
			bt_peer_send_handshake (self);
		}

		self->status = BT_PEER_STATUS_HANDSHAKE_PEER_ID;
	}

	if (self->status == BT_PEER_STATUS_HANDSHAKE_PEER_ID) {
		if (self->pos < 20)
			goto wait;

		g_string_erase (self->buffer, 0, 20);

		self->pos -= 20;

		g_debug ("got peer id");

		self->status = BT_PEER_STATUS_IDLE_HAVE;
	}

	if (self->status == BT_PEER_STATUS_IDLE_HAVE) {
		
	}

	return;

cleanup:
	gnet_conn_disconnect (self->socket);
	gnet_conn_delete (self->socket);
	self->socket = NULL;
	return;

wait:
	gnet_conn_read (self->socket);
	return;
}

static void
bt_peer_connection_callback (GConn *connection G_GNUC_UNUSED, GConnEvent *event, BtPeer *self)
{
	g_return_if_fail (BT_IS_PEER (self));

	switch (event->type) {
	case GNET_CONN_ERROR:
		g_warning ("connection error");
		break;

	case GNET_CONN_CONNECT:
		g_debug ("connected");
		self->status = BT_PEER_STATUS_HANDSHAKE;
		bt_peer_send_handshake (self);
		break;

	case GNET_CONN_READ:
		bt_peer_receive (self, event->buffer, event->length);
		break;

	case GNET_CONN_WRITE:
		switch (self->send_status) {
		case BT_PEER_SEND_STATUS_HANDSHAKING:
			self->send_status = BT_PEER_SEND_STATUS_IDLE;
			self->status = BT_PEER_STATUS_HANDSHAKE;
			gnet_conn_read (self->socket);
			break;

		default:
			break;
		}
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
	self->send_status = BT_PEER_SEND_STATUS_HANDSHAKE;
	gnet_conn_connect (self->socket);
}

BtPeer *
bt_peer_new (BtManager *manager, BtTorrent *torrent, GTcpSocket *socket, GInetAddr *address)
{
	BtPeer *self;

	g_return_val_if_fail (BT_IS_MANAGER (manager), NULL);

	g_return_val_if_fail ((torrent && !socket && address) || (!torrent && socket && !address), NULL);

	self = BT_PEER (g_object_new (BT_TYPE_PEER, NULL));

	self->manager = g_object_ref (manager);

	if (socket) {
		self->socket = gnet_conn_new_socket (socket, (GConnFunc) bt_peer_connection_callback, self);
		self->address = gnet_tcp_socket_get_remote_inetaddr (socket);
		self->status = BT_PEER_STATUS_HANDSHAKE;
		self->send_status = BT_PEER_SEND_STATUS_HANDSHAKE;
		gnet_conn_read (self->socket);
	} else {
		self->address = address;
		self->socket = gnet_conn_new_inetaddr (address, (GConnFunc) bt_peer_connection_callback, self);
		self->torrent = g_object_ref (torrent);
		self->status = BT_PEER_STATUS_DISCONNECTED;
		self->send_status = BT_PEER_SEND_STATUS_DISCONNECTED;
		bt_peer_connect (self);
	}

	gnet_conn_set_watch_error (self->socket, TRUE);

	return self;
}
