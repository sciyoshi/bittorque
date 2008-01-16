/**
 * bt-manager.c
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

#include <gnet.h>

#include "bt-manager.h"
#include "bt-peer.h"
#include "bt-utils.h"

enum {
	BT_MANAGER_PROPERTY_PORT = 1,
	BT_MANAGER_PROPERTY_PEER_ID
};

enum {
	BT_MANAGER_SIGNAL_NEW_CONNECTION,
	BT_MANAGER_NUM_SIGNALS
};

static guint bt_manager_signals[BT_MANAGER_NUM_SIGNALS] = {0};

struct _BtManager {
	GObject parent;
	
	/* port to listen for incoming connections on */
	gint port;
	
	/* the peer id to use for torrents */
	gchar *peer_id;
	
	/* the listening socket */
	GTcpSocket *listen_socket;
	
	/* hash table mapping infohash -> torrent */
	GHashTable *torrents;
};

struct _BtManagerClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (BtManager, bt_manager, G_TYPE_OBJECT)

static void
bt_manager_new_connection (BtManager *manager, GTcpSocket *socket, gpointer data G_GNUC_UNUSED)
{
	BtPeer *peer;

	g_return_if_fail (socket != NULL);

	g_debug ("received new connection");
	
	peer = bt_peer_new_incoming (manager, socket);

	return;
}

static void
bt_manager_emit_new_connection (GTcpSocket *server G_GNUC_UNUSED, GTcpSocket *client, BtManager *manager)
{
	g_return_if_fail (BT_IS_MANAGER (manager));

	g_signal_emit (manager, bt_manager_signals[BT_MANAGER_SIGNAL_NEW_CONNECTION], 0, client);

	return;
}

/**
 * bt_manager_add_torrent:
 * @manager: the manager
 * @torrent: the torrent to add
 *
 * Add a torrent to the manager.
 */
void
bt_manager_add_torrent (BtManager *manager, BtTorrent *torrent)
{
	g_return_if_fail (BT_IS_MANAGER (manager));

	g_hash_table_insert (manager->torrents, g_strdup (bt_torrent_get_infohash_string (torrent)), g_object_ref (torrent));
}

/**
 * bt_manager_get_torrent:
 * @manager: the manager
 * @infohash: the torrent's infohash to add
 *
 * Get a torrent from the manager with the specified infohash, or NULL.
 */
BtTorrent *
bt_manager_get_torrent (BtManager *manager, gchar *infohash)
{
	BtTorrent *torrent;
	gchar *infohash_string;

	g_return_val_if_fail (BT_IS_MANAGER (manager), NULL);
	g_return_val_if_fail (infohash != NULL, NULL);

	infohash_string = bt_hash_to_string (infohash);

	torrent = (BtTorrent *) g_hash_table_lookup (manager->torrents, infohash_string);
	
	g_free (infohash_string);
	
	return torrent;
}

/**
 * bt_manager_stop_accepting:
 * @manager: the manager
 *
 * Stop accepting incoming connections.
 */
void
bt_manager_stop_accepting (BtManager *manager)
{
	g_return_if_fail (BT_IS_MANAGER (manager));
	
	if (manager->listen_socket == NULL)
		return;
	
	gnet_tcp_socket_delete (manager->listen_socket);
	
	manager->listen_socket = NULL;
	
	return;
}

/**
 * bt_manager_start_accepting:
 * @manager: the manager
 * @error: return location for errors
 *
 * Start accepting incoming connections.
 *
 * Returns: TRUE on error, otherwise FALSE and @error is set.
 */
gboolean
bt_manager_start_accepting (BtManager *manager, GError **error)
{
	g_return_val_if_fail (BT_IS_MANAGER (manager), FALSE);

	if (manager->listen_socket != NULL)
		bt_manager_stop_accepting (manager);

	manager->listen_socket = gnet_tcp_socket_server_new_with_port (manager->port);

	if (!manager->listen_socket) {
		g_set_error (error, BT_ERROR, BT_ERROR_NETWORK, "error creating listening socket");
		return FALSE;
	}

	gnet_tcp_socket_server_accept_async (manager->listen_socket, (GTcpSocketAcceptFunc) bt_manager_emit_new_connection, manager);

	return TRUE;
}

/**
 * bt_manager_get_port:
 * @manager: the manager
 *
 * Gets the port that this manager is listening on.
 *
 * Returns: the port.
 */
gushort
bt_manager_get_port (BtManager *manager)
{
	g_return_val_if_fail (BT_IS_MANAGER (manager), 0);

	return manager->port;
}

/**
 * bt_manager_set_port:
 * @manager: the manager
 * @port: the new port
 *
 * Sets the port that this manager should listen on for incoming
 * connections. If it is already running, it will be restarted
 * to listen on this new port.
 */
void
bt_manager_set_port (BtManager *manager, gushort port)
{
	GError *error = NULL;
	gboolean running = FALSE;

	g_return_if_fail (BT_IS_MANAGER (manager));

	if (manager->port == port)
		return;

	g_object_freeze_notify (G_OBJECT (manager));	

	if (manager->listen_socket != NULL) {
		running = TRUE;
		bt_manager_stop_accepting (manager);
	}

	manager->port = port;
	
	if (running) {
		if (!bt_manager_start_accepting (manager, &error)) {
			g_warning ("could not start accepting connections: %s", error->message);
			g_clear_error (&error);
		}
	}

	g_object_notify (G_OBJECT (manager), "port");

	g_object_thaw_notify (G_OBJECT (manager));

	return;
}

/**
 * bt_manager_get_peer_id:
 * @manager: the manager
 *
 * Gets the peer id.
 *
 * Returns: the peer id. This is a 20-byte string that must not
 * be modified, freed, or stored.
 */
const gchar *
bt_manager_get_peer_id (BtManager *manager)
{
	return manager->peer_id;
}

/**
 * bt_manager_set_peer_id:
 * @manager: the manager
 * @peer_id: the new peer id to set
 *
 * Sets the peer id for all the torrents that this manager owns.
 * @peer_id must be 20 bytes long.
 */
void
bt_manager_set_peer_id (BtManager *manager, const gchar *peer_id)
{
	g_return_if_fail (BT_IS_MANAGER (manager));

	/* TODO: is this needed? */
	g_object_freeze_notify (G_OBJECT (manager));	

	if (manager->peer_id != NULL)
		g_free (manager->peer_id);

	manager->peer_id = g_strdup (peer_id);

	g_object_notify (G_OBJECT (manager), "peer_id");

	/* TODO: is this needed? */
	g_object_thaw_notify (G_OBJECT (manager));

	return;
}

static void
bt_manager_clear_torrents (gpointer key G_GNUC_UNUSED, gpointer value, gpointer user_data G_GNUC_UNUSED)
{
	g_object_unref (G_OBJECT (value));
}

static void
bt_manager_dispose (GObject *object)
{
	BtManager *self = BT_MANAGER (object);
	
	if (self->torrents == NULL)
		return;

	g_hash_table_foreach (self->torrents, &bt_manager_clear_torrents, NULL);
	g_hash_table_unref (self->torrents);
	
	self->torrents = NULL;
	
	G_OBJECT_CLASS (bt_manager_parent_class)->dispose (object);
}

static void
bt_manager_finalize (GObject *object)
{
	BtManager *self = BT_MANAGER (object);
	
	g_free (self->peer_id);

	G_OBJECT_CLASS (bt_manager_parent_class)->finalize (object);
}

static void
bt_manager_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec)
{
	BtManager *self = BT_MANAGER (object);
	
	switch (property) {
	case BT_MANAGER_PROPERTY_PORT:
		bt_manager_set_port (self, g_value_get_int (value));
		break;
	
	case BT_MANAGER_PROPERTY_PEER_ID:
		bt_manager_set_peer_id (self, g_value_get_string (value));
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
}

static void
bt_manager_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec)
{
	BtManager *self = BT_MANAGER (object);
	
	switch (property) {
	case BT_MANAGER_PROPERTY_PORT:
		g_value_set_int (value, self->port);
		break;
		
	case BT_MANAGER_PROPERTY_PEER_ID:
		g_value_set_string (value, self->peer_id);
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
}

static void
bt_manager_init (BtManager *manager)
{
	manager->torrents = g_hash_table_new (g_str_hash, g_str_equal);

	return;
}

static void
bt_manager_class_init (BtManagerClass *manager_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (manager_class);
	gchar* default_id = bt_create_peer_id ();
	GParamSpec *pspec;
	GClosure *closure;
	GType params[2];
	
	object_class->dispose = bt_manager_dispose;
	object_class->finalize = bt_manager_finalize;
	object_class->set_property = bt_manager_set_property;
	object_class->get_property = bt_manager_get_property;

	/**
	 * BtManager:port:
	 *
	 * The port that this manager should listen on for incoming connections.
	 */
	pspec = g_param_spec_int ("port",
	                          "listening port",
	                          "Port on which to listen for incoming connections",
	                          1024,
	                          65535,
	                          6881,
	                          G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK | G_PARAM_CONSTRUCT);

	g_object_class_install_property (object_class, BT_MANAGER_PROPERTY_PORT, pspec);

	/**
	 * BtManager:peer-id:
	 *
	 * The peer id that torrents should send to peers and trackers. 
	 */
	pspec = g_param_spec_string ("peer-id",
	                             "peer id",
	                             "The peer id to use for torrent connections",
	                             default_id,
	                             G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK | G_PARAM_CONSTRUCT);

	g_free (default_id);

	g_object_class_install_property (object_class, BT_MANAGER_PROPERTY_PEER_ID, pspec);
	
	/**
	 * BtManager::new-connection:
	 *
	 * Emitted when a new connection has been received.
	 */
	closure = g_cclosure_new (G_CALLBACK (bt_manager_new_connection), NULL, NULL);
	
	params[0] = G_TYPE_POINTER;
	 
	bt_manager_signals[BT_MANAGER_SIGNAL_NEW_CONNECTION] =
		g_signal_newv ("new-connection",
		               G_TYPE_FROM_CLASS (object_class),
		               G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
		               closure,
		               NULL,
		               NULL,
		               g_cclosure_marshal_VOID__POINTER,
		               G_TYPE_NONE,
		               1,
		               params);

	return;
}
