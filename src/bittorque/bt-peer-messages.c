#include <string.h>

#include "bt-utils.h"
#include "bt-peer.h"

gboolean
bt_peer_parse_handshake (BtPeer *self, GError **error G_GNUC_UNUSED)
{
	gchar *buf;

	if (self->pos < 48) {
		gnet_conn_read (self->socket);
		return FALSE;
	}

	g_print ("handshake done");

	buf = self->buffer->str;

	/* FIXME: change this to \19, this is just for testing */
	if (buf[0] != 'a')
		goto bad;

	if (strncmp (buf + 1, "BitTorrent protocol", 19) != 0)
		goto bad;

	g_string_erase (self->buffer, 0, 48);
	self->pos -= 48;

	return TRUE;

  bad:
	g_set_error (error, BT_ERROR, BT_ERROR_PEER_HANDSHAKE, "invalid handshake sent by peer");
	return FALSE;
}

void
bt_peer_send_handshake (BtPeer *self G_GNUC_UNUSED)
{
	return;
}

void
bt_peer_receive (BtPeer *self, gchar *buf, gsize len)
{
	GError *error;

	g_return_if_fail (BT_IS_PEER (self));

	error = NULL;

	if (len == 0) {
		g_debug ("connection closed");
		return;
	}

	g_string_append_len (self->buffer, buf, len);
	self->pos += len;

	g_print ("%s\n", self->buffer->str);

	switch (self->status) {
	case BT_PEER_STATUS_CONNECTED_WAIT:
		if (!bt_peer_parse_handshake (self, &error)) {
			if (error != NULL) {
				if (g_error_matches (error, BT_ERROR, BT_ERROR_PEER_HANDSHAKE)) {
					/* TODO: try encryption */
				}
				g_warning (error->message);
				g_clear_error (&error);
			}
			break;
		}
		bt_peer_send_handshake (self);
		self->status = BT_PEER_STATUS_SEND_HANDSHAKE;
		break;

	case BT_PEER_STATUS_WAIT_HANDSHAKE:
		if (!bt_peer_parse_handshake (self, &error)) {
			if (error != NULL) {
				g_warning (error->message);
				g_clear_error (&error);
			}
			break;
		}
		self->status = BT_PEER_STATUS_IDLE;
		break;

	default:
		break;
	}

	return;
}
