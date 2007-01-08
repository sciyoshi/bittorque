/**
 * bt-torrent-announce.c
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
#include <stdlib.h>

#include <gnet.h>

#include "bt-manager.h"
#include "bt-peer.h"
#include "bt-torrent.h"
#include "bt-bencode.h"
#include "bt-utils.h"

static gboolean
bt_torrent_announce_http_parse_response (BtTorrent *self, const gchar *buf)
{
	GError *error;
	BtBencode *response, *failure, *warning, *interval, *tracker_id, *peers;

	g_return_val_if_fail (BT_IS_TORRENT (self), FALSE);

	error = NULL;

	if (!(response = bt_bencode_decode (buf, &error))) {
		g_warning ("could not decode tracker response: %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	failure = bt_bencode_lookup (response, "failure reason");

	if (failure) {
		if (failure->type == BT_BENCODE_TYPE_STRING)
			g_warning ("tracker sent error: %s", failure->string->str);
		return FALSE;
	}

	warning = bt_bencode_lookup (response, "warning message");

	if (warning && warning->type == BT_BENCODE_TYPE_STRING)
		g_warning ("tracker sent warning: %s", warning->string->str);

	interval = bt_bencode_lookup (response, "interval");

	if (interval && interval->type == BT_BENCODE_TYPE_INT) {
		self->tracker_interval = interval->value;
		g_debug ("tracker announce interval: %d", self->tracker_interval);
	}

	interval = bt_bencode_lookup (response, "min interval");

	if (interval && interval->type == BT_BENCODE_TYPE_INT) {
		self->tracker_min_interval = interval->value;
		g_debug ("tracker announce mininmum interval: %d", self->tracker_min_interval);
	}

	tracker_id = bt_bencode_lookup (response, "tracker id");

	if (tracker_id && tracker_id->type == BT_BENCODE_TYPE_STRING) {
		self->tracker_id = g_strdup (tracker_id->string->str);
		g_debug ("tracker id: %s", self->tracker_id);
	}

	peers = bt_bencode_lookup (response, "peers");

	if (peers && peers->type == BT_BENCODE_TYPE_STRING) {
		int num, i;
		
		if (peers->string->len % 6 != 0)
			g_warning ("invalid peers string");
		
		num = peers->string->len / 6;
		
		for (i = 0; i < num; i++) {
			GInetAddr *address;
			address = gnet_inetaddr_new_bytes (peers->string->str + i * 6, 4);
			gnet_inetaddr_set_port (address, g_ntohs (*((gushort *) (peers->string->str + i * 6 + 4))));
			bt_torrent_add_peer (self, bt_peer_new (self->manager, self, NULL, address));
			gnet_inetaddr_unref (address);
		}
	}


	return TRUE;
}

static void
bt_torrent_announce_http_callback (GConnHttp *connection G_GNUC_UNUSED, GConnHttpEvent *event, BtTorrent *self)
{
	GConnHttpEventData *data;
	GConnHttpEventResponse *response;
	gchar *buf;
	gsize len;

	g_return_if_fail (BT_IS_TORRENT (self));

	switch (event->type) {
	case GNET_CONN_HTTP_RESOLVED:
		g_debug ("tracker address resolved");
		break;

	case GNET_CONN_HTTP_RESPONSE:
		response = (GConnHttpEventResponse *) event;
		g_debug ("tracker sent response with code: %d", response->response_code);
		break;

	case GNET_CONN_HTTP_DATA_COMPLETE:
		data = (GConnHttpEventData *) event;
		gnet_conn_http_steal_buffer (connection, &buf, &len);
		bt_torrent_announce_http_parse_response (self, buf);
		g_free (buf);
		bt_idle_source_create (self->manager, (GSourceFunc) bt_torrent_announce_stop, self);
		break;

	case GNET_CONN_HTTP_ERROR:
		g_warning ("tracker conection error");
		bt_idle_source_create (self->manager, (GSourceFunc) bt_torrent_announce_stop, self);

	default:
		break;
	}

	return;
}

static gboolean
bt_torrent_announce_http (BtTorrent *self, gchar *announce)
{
	gchar *query, *tmp;

	g_return_val_if_fail (BT_IS_TORRENT (self), FALSE);

	g_return_val_if_fail (self->tracker_socket == NULL, FALSE);

	/* build query */
	tmp = bt_url_encode (self->infohash, 20);

	query = g_strdup_printf ("%s?info_hash=%s&peer_id=%s&port=%d&uploaded=%d&downloaded=%d&left=%lld&compact=1&event=%s&numwant=%d",
	                         announce,
	                         tmp,
	                         self->manager->peer_id,
	                         bt_manager_get_port (self->manager),
	                         0,
	                         0,
	                         self->size,
	                         "started",
	                         30);

	g_debug ("hitting tracker with query %s", query);

	g_free (tmp);

	self->tracker_socket = gnet_conn_http_new ();

	if (!gnet_conn_http_set_escaped_uri (self->tracker_socket, query)) {
		gnet_conn_http_delete (self->tracker_socket);
		self->tracker_socket = NULL;
		g_warning ("could not accept uri");
		return FALSE;
	}

	gnet_conn_http_run_async (self->tracker_socket, (GConnHttpFunc) bt_torrent_announce_http_callback, self);

	g_free (query);

	return TRUE;
}

static gboolean
bt_torrent_announce_udp (BtTorrent *self G_GNUC_UNUSED, gchar *announce G_GNUC_UNUSED)
{
	return FALSE;
}

static gboolean
bt_torrent_announce_single (BtTorrent *self, gchar *announce)
{
	g_return_val_if_fail (BT_IS_TORRENT (self), FALSE);

	/* find the type of protocol to use */
	if (g_ascii_strncasecmp (announce, "http", 4) == 0) {
		if (!bt_torrent_announce_http (self, announce))
			return FALSE;
	} else if (g_ascii_strncasecmp (announce, "udp", 3) == 0) {
		if (strncmp (announce + 3, "://", 3) != 0) {
			g_warning ("invalid announce string");
			return FALSE;
		}
		if (!bt_torrent_announce_udp (self, announce + 6))
			return FALSE;
	} else {
		g_warning ("unsupported tracker protocol");
		return FALSE;
	}

	return TRUE;
}

gboolean
bt_torrent_announce (BtTorrent *self)
{
	g_return_val_if_fail (BT_IS_TORRENT (self), FALSE);

	if (self->announce_list) {
		GSList *tier = g_slist_nth (self->announce_list, self->tracker_current_tier);
		GSList *tracker = g_slist_nth ((GSList *) tier->data, self->tracker_current_tracker);
		bt_torrent_announce_single (self, (gchar *) tracker->data);
/*			self->tracker_current_tracker++;
			if (self->tracker_current_tracker >= g_slist_length (tier)) {
				self->tracker_current_tracker = 0;
				self->tracker_current_tier++;
				if (self->tracker_current_tier >= g_slist_length (self->announce_list))
					g_warning ("could not reach any of the trackers");
			}
		} else {
			if (j != i->data) {
				i->data = g_slist_remove_link ((GSList *) i->data, j);
				i->data = g_slist_concat (j, (GSList *) i->data);
			}
		}*/
	} else {
		bt_torrent_announce_single (self, self->announce);
	}

	return FALSE;
}

gboolean
bt_torrent_announce_stop (BtTorrent *self)
{
	g_return_val_if_fail (BT_IS_TORRENT (self), FALSE);

	if (!self->tracker_socket)
		return FALSE;

	gnet_conn_http_delete (self->tracker_socket);
	self->tracker_socket = NULL;
	return FALSE;
}
