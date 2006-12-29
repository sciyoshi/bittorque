/**
 * bt-peer-messages.c
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

#include "bt-utils.h"
#include "bt-peer.h"

void
bt_peer_send_keepalive (BtPeer *self)
{
	static gchar buf[5] = {0, 0, 0, 0};
	
	gnet_conn_write (self->socket, buf, 4);
}

void
bt_peer_send_choke (BtPeer *self)
{
	static gchar buf[5] = {0, 0, 0, 1, 0};
	
	gnet_conn_write (self->socket, buf, 5);
}

void
bt_peer_send_unchoke (BtPeer *self)
{
	static gchar buf[5] = {0, 0, 0, 1, 1};
	
	gnet_conn_write (self->socket, buf, 5);
}


void
bt_peer_send_interested (BtPeer *self)
{
	static gchar buf[5] = {0, 0, 0, 1, 2};
	
	gnet_conn_write (self->socket, buf, 5);
}

void
bt_peer_send_uninterested (BtPeer *self)
{
	static gchar buf[5] = {0, 0, 0, 1, 3};
	
	gnet_conn_write (self->socket, buf, 5);
}

void
bt_peer_send_handshake (BtPeer *self)
{
	gchar buf[68];

	g_return_if_fail (BT_IS_PEER (self) && BT_IS_TORRENT (self->torrent));

	buf[0] = (char) 19;

	memcpy (buf + 1, "BitTorrent protocol", 19);

	memset (buf + 20, '\0', 8);

	memcpy (buf + 28, self->torrent->infohash, 20);

	memcpy (buf + 48, self->manager->peer_id, 20);

	gnet_conn_write (self->socket, buf, 68);

	if (self->status == BT_PEER_STATUS_CONNECTED_SEND) {
		gnet_conn_read (self->socket);
	}

	return;
}

gboolean
bt_peer_parse_handshake (BtPeer *self, GError **error)
{
	gchar *buf;

	if (self->pos < 48) {
		/* we need more */
		gnet_conn_read (self->socket);
		return FALSE;
	}

	buf = self->buffer->str;

	if (buf[0] != (char) 19)
		goto bad;

	if (strncmp (buf + 1, "BitTorrent protocol", 19) != 0)
		goto bad;

	self->torrent = bt_manager_get_torrent (self->manager, buf + 28);

	if (!self->torrent) {
		g_set_error (error, BT_ERROR, BT_ERROR_PEER_HANDSHAKE, "connection received for torrent we are not serving");
		return FALSE;
	}

	g_object_ref (self->torrent);

	g_string_erase (self->buffer, 0, 48);
	self->pos -= 48;

	g_debug ("valid handshake received");

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
bt_peer_receive (BtPeer *self, gchar *buf, gsize len, gpointer data G_GNUC_UNUSED)
{
	GError *error = NULL;

	g_return_if_fail (BT_IS_PEER (self));

	if (len == 0) {
		g_debug ("connection closed");
		return;
	}

	g_string_append_len (self->buffer, buf, len);
	self->pos += len;

	switch (self->status) {
	case BT_PEER_STATUS_CONNECTED_WAIT:
		if (!bt_peer_parse_handshake (self, &error)) {
			if (error != NULL) {
				if (g_error_matches (error, BT_ERROR, BT_ERROR_PEER_HANDSHAKE)) {

				}
				g_warning (error->message);
				g_clear_error (&error);
				bt_idle_source_create (self->manager, (GSourceFunc) bt_peer_disconnect, self);
			}
			break;
		}

		break;

	case BT_PEER_STATUS_WAIT_HANDSHAKE:
		if (!bt_peer_parse_handshake (self, &error)) {
			if (error != NULL) {
				g_warning (error->message);
				g_clear_error (&error);
				bt_idle_source_create (self->manager, (GSourceFunc) bt_peer_disconnect, self);
			}
			break;
		}
		break;

	default:
		break;
	}

	return;
}
