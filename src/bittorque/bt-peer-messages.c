#include <string.h>

#include "bt-utils.h"
#include "bt-peer.h"

void
bt_peer_send_handshake (BtPeer *self)
{
	gchar buf[68];

	g_return_if_fail (BT_IS_PEER (self) && BT_IS_TORRENT (self->torrent));

	buf[0] = '\19';

	memcpy (buf + 1, "BitTorrent protocol", 19);

	memset (buf + 20, '\0', 8);

	memcpy (buf + 28, self->torrent->infohash, 20);

	memcpy (buf + 48, self->manager->peer_id, 20);

	gnet_conn_write (self->socket, buf, 68);

	self->status = BT_PEER_STATUS_WAIT_HANDSHAKE;

	return;
}

gboolean
bt_peer_parse_handshake (BtPeer *self, GError **error G_GNUC_UNUSED)
{
	gchar *buf;

	if (self->pos < 48) {
		gnet_conn_read (self->socket);
		return FALSE;
	}

	buf = self->buffer->str;

	if (buf[0] != '\19')
		goto bad;

	if (strncmp (buf + 1, "BitTorrent protocol", 19) != 0)
		goto bad;

	self->torrent = bt_manager_get_torrent (self->manager, buf + 28);

	if (!self->torrent) {
		g_set_error (error, BT_ERROR, BT_ERROR_PEER_HANDSHAKE, "connection received for torrent we are not serving");
		goto bad;
	}

	g_object_ref (self->torrent);

	g_string_erase (self->buffer, 0, 48);
	self->pos -= 48;

	if (self->status == BT_PEER_STATUS_CONNECTED_WAIT) {
		bt_peer_send_handshake (self);
	}

	self->status = BT_PEER_STATUS_IDLE;

	return TRUE;

  bad:
	g_set_error (error, BT_ERROR, BT_ERROR_PEER_HANDSHAKE, "invalid handshake sent by peer");
	return FALSE;
}

void
bt_peer_receive (BtPeer *self, gchar *buf, gsize len)
{
	GError *error = NULL;

	g_return_if_fail (BT_IS_PEER (self));

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
					/* FIXME: try encryption */
				}
				g_warning (error->message);
				g_clear_error (&error);
			}
			break;
		}

		break;

	case BT_PEER_STATUS_WAIT_HANDSHAKE:
		if (!bt_peer_parse_handshake (self, &error)) {
			if (error != NULL) {
				g_warning (error->message);
				g_clear_error (&error);
			}
			break;
		}
		break;

	default:
		break;
	}

	return;
}
