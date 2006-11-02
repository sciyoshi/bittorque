#include <glib.h>
#include <gnet.h>

#include "bt-manager.h"
#include "bt-utils.h"
#include "bt-peer.h"

G_DEFINE_TYPE (BtManager, bt_manager, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_PORT,
};

static void
bt_manager_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtManager *self = BT_MANAGER (object);
	GError *error = NULL;

	switch (property) {
	case PROP_PORT:
		if (!bt_manager_set_port (self, g_value_get_uint (value), &error)) {
			g_warning ("could not set port: %s", error->message);
			g_clear_error (&error);
		}
	}
}

static void
bt_manager_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtManager *self = BT_MANAGER (object);

	switch (property) {
	case PROP_PORT:
		g_value_set_uint (value, bt_manager_get_port (self));
	}
}

static void
bt_manager_init (BtManager *manager)
{
	manager->context = NULL;
	manager->accept_socket = NULL;
	manager->port = 6881;
	manager->torrents = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) g_object_unref);
	bt_create_peer_id (manager->peer_id);
}

static void
bt_manager_dispose (GObject *manager)
{
	BtManager *self = BT_MANAGER (manager);

	if (!self->context && !self->accept_socket && !self->torrents)
		return;

	if (self->context) {
		g_main_context_unref (self->context);
		self->context = NULL;
	}

	if (self->accept_socket) {
		bt_manager_accept_stop (self);
	}

	if (self->torrents) {
		g_hash_table_unref (self->torrents);
		self->torrents = NULL;
	}

	((GObjectClass *) bt_manager_parent_class)->dispose (G_OBJECT (self));
}

static void
bt_manager_finalize (GObject *manager)
{
	((GObjectClass *) bt_manager_parent_class)->finalize (manager);
}

static void
bt_manager_class_init (BtManagerClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;

	if (!g_thread_supported ())
		g_thread_init (NULL);

	gclass = G_OBJECT_CLASS (klass);

	gclass->set_property = bt_manager_set_property;
	gclass->get_property = bt_manager_get_property;
	gclass->finalize     = bt_manager_finalize;
	gclass->dispose      = bt_manager_dispose;

	pspec = g_param_spec_uint ("port",
	                           "accept port",
	                           "The port to use to listen for peer connections on.",
	                           0,
	                           G_MAXUSHORT,
	                           6881,
	                           G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_PORT, pspec);
}

static void
bt_manager_accept_callback (GTcpSocket* server G_GNUC_UNUSED, GTcpSocket* client, BtManager *self)
{
	GInetAddr *addr;
	gushort    port;
	gchar     *name;

	g_return_if_fail (BT_IS_MANAGER (self));

	if (!client) {
		g_warning ("could not accept connection");
		return;
	}

	addr = gnet_tcp_socket_get_local_inetaddr (client);
	name = gnet_inetaddr_get_canonical_name (addr);
	port = gnet_inetaddr_get_port (addr);

	g_debug ("connection received from %s, port %d", name, port);

	bt_peer_new (self, NULL, client, NULL);

	return;
}

gboolean
bt_manager_accept_start (BtManager *self, GError **error)
{
	g_return_val_if_fail (BT_IS_MANAGER (self), FALSE);

	self->accept_socket = gnet_tcp_socket_server_new_with_port (self->port);

	if (!self->accept_socket) {
		g_set_error (error, BT_ERROR, BT_ERROR_NETWORK, "could not set up listening socket");
		return FALSE;
	}

	gnet_tcp_socket_server_accept_async (self->accept_socket, (GTcpSocketAcceptFunc) bt_manager_accept_callback, self);

	return TRUE;
}

void
bt_manager_accept_stop (BtManager *self)
{
	g_return_if_fail (BT_IS_MANAGER (self));

	gnet_tcp_socket_delete (self->accept_socket);

	self->accept_socket = NULL;
}


guint
bt_manager_add_source (BtManager *self, GSource *source)
{
	return g_source_attach (source, self->context);
}

/**
 * bt_manager_set_port:
 *
 * Set the port that this manager should listen on for peer connections. If the manager is already
 * running, it will be restarted.
 */

gboolean
bt_manager_set_port (BtManager *self, gushort port, GError **error)
{
	gboolean running;

	if (self->port == port)
		return TRUE;

	running = self->accept_socket != NULL ? TRUE : FALSE;

	if (running)
		bt_manager_accept_stop (self);

	self->port = port;

	if (running)
		if (!bt_manager_accept_start (self, error))
			return FALSE;

	return TRUE;
}

gushort
bt_manager_get_port (BtManager *self)
{
	return self->port;
}

void
bt_manager_add_torrent (BtManager *self, BtTorrent *torrent)
{
	g_hash_table_insert (self->torrents, torrent->infohash_string, g_object_ref (torrent));
}

BtTorrent *
bt_manager_get_torrent (BtManager *self, gchar *infohash)
{
	gchar string[41];
	bt_hash_to_string (infohash, string);
	return bt_manager_get_torrent_string (self, string);
}

BtTorrent *
bt_manager_get_torrent_string (BtManager *self, gchar *infohash)
{
	return (BtTorrent *) g_hash_table_lookup (self->torrents, infohash);
}

BtManager *
bt_manager_new ()
{
	return bt_manager_new_with_main (NULL);
}

BtManager *
bt_manager_new_with_main (GMainContext *context)
{
	BtManager *manager = BT_MANAGER (g_object_new (BT_TYPE_MANAGER, NULL));

	if (context)
		g_main_context_ref (context);

	manager->context = context;

	return manager;
}
