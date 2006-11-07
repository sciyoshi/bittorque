#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gnet.h>

#include "bt-manager.h"
#include "bt-utils.h"
#include "bt-peer.h"

G_DEFINE_TYPE (BtManager, bt_manager, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_CONTEXT,
	PROP_PREFERENCES,
	PROP_PORT,
	PROP_PEER_ID,
	PROP_PRIVATE_DIR
};

enum {
	SIGNAL_NEW_CONNECTION,
	SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

static void
bt_manager_new_connection_callback (BtManager *self, GTcpSocket *client, gpointer data G_GNUC_UNUSED)
{
	GInetAddr *addr;
	gushort port;
	gchar *name;

	g_return_if_fail (BT_IS_MANAGER (self));

	g_return_if_fail (client);

	addr = gnet_tcp_socket_get_remote_inetaddr (client);
	name = gnet_inetaddr_get_canonical_name (addr);
	port = gnet_inetaddr_get_port (addr);

	g_debug ("connection received from %s, port %d", name, port);

	g_free (name);
	gnet_inetaddr_delete (addr);

	bt_peer_new (self, NULL, client, NULL);

	return;
}

static void
bt_manager_accept_callback (GTcpSocket *server G_GNUC_UNUSED, GTcpSocket *client, BtManager *self)
{
	g_return_if_fail (BT_IS_MANAGER (self));

	if (!client) {
		g_warning ("could not accept connection");
		return;
	}

	g_signal_emit (self, signals[SIGNAL_NEW_CONNECTION], 0, client);

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

	gnet_tcp_socket_server_accept_async (self->accept_socket, (GTcpSocketAcceptFunc)
					     bt_manager_accept_callback, self);

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
bt_manager_new_with_main_preferences (GMainContext *context, GKeyFile *preferences)
{
	return BT_MANAGER (g_object_new (BT_TYPE_MANAGER, "context", context, "preferences", preferences, NULL));
}

BtManager *
bt_manager_new ()
{
	return bt_manager_new_with_main_preferences (NULL, NULL);
}

BtManager *
bt_manager_new_with_preferences (GKeyFile *preferences)
{
	return bt_manager_new_with_main_preferences (NULL, preferences);
}

BtManager *
bt_manager_new_with_main (GMainContext *context)
{
	return bt_manager_new_with_main_preferences (context, NULL);
}

static void
bt_manager_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtManager *self = BT_MANAGER (object);
	GError *error = NULL;
	const gchar *string;

	switch (property) {
	case PROP_PORT:
		if (!bt_manager_set_port (self, g_value_get_uint (value), &error)) {
			g_warning ("could not set port: %s", error->message);
			g_clear_error (&error);
		}
		break;

	case PROP_CONTEXT:
		if (self->context)
			g_main_context_unref (self->context);
		self->context = g_value_get_pointer (value);
		if (self->context)
			g_main_context_ref (self->context);
		break;

	case PROP_PREFERENCES:
		self->preferences = g_value_get_pointer (value);
		if (!self->preferences) {
			self->preferences = g_key_file_new ();
			/* FIXME: load default configuration from system directory */
		}
		break;

	case PROP_PEER_ID:
		string = g_value_dup_string (value);
		if (strlen (string) != 20)
			return;
		memcpy (self->peer_id, string, 20);
		break;

	case PROP_PRIVATE_DIR:
		if (self->private_dir)
			g_free (self->private_dir);
		self->private_dir = g_value_dup_string (value);
		break;
	}
}

static void
bt_manager_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtManager *self = BT_MANAGER (object);

	switch (property) {
	case PROP_PORT:
		g_value_set_uint (value, bt_manager_get_port (self));
		break;

	case PROP_CONTEXT:
		g_value_set_pointer (value, self->context);
		break;

	case PROP_PREFERENCES:
		g_value_set_pointer (value, self->context);
		break;

	case PROP_PEER_ID:
		g_value_take_string (value, g_strndup (self->peer_id, 20));
		break;

	case PROP_PRIVATE_DIR:
		g_value_set_string (value, self->private_dir);
		break;
	}
}

static void
bt_manager_init (BtManager *manager)
{
	manager->context = NULL;
	manager->accept_socket = NULL;
	manager->private_dir = g_strdup ("~/.bittorque");
	manager->port = 6881;
	manager->torrents = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify) g_object_unref);
	memset (manager->peer_id, 21, 0);
}

static GObject *
bt_manager_constructor (GType type, guint num, GObjectConstructParam *properties)
{
	GObject *object;
	BtManager *self;

	object = G_OBJECT_CLASS (bt_manager_parent_class)->constructor (type, num, properties);
	self = BT_MANAGER (object);

	if (*(self->peer_id) == '\0')
		bt_create_peer_id (self->peer_id);

	return object;
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
	BtManager *self = BT_MANAGER (manager);

	g_key_file_free (self->preferences);

	((GObjectClass *) bt_manager_parent_class)->finalize (manager);
}

static void
bt_manager_class_init (BtManagerClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;
	GClosure *closure;
	GType params[1];

	if (!g_thread_supported ())
		g_thread_init (NULL);

	gclass = G_OBJECT_CLASS (klass);

	gclass->set_property = bt_manager_set_property;
	gclass->get_property = bt_manager_get_property;
	gclass->finalize = bt_manager_finalize;
	gclass->dispose = bt_manager_dispose;
	gclass->constructor = bt_manager_constructor;

	pspec = g_param_spec_pointer ("context",
	                              "main context",
	                              "The GMainContext that this manager should run in.",
	                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_CONTEXT, pspec);

	pspec = g_param_spec_pointer ("preferences",
	                              "torrent manager preferences",
	                              "A pointer to a GKeyFile storing manager preferences.",
	                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_PREFERENCES, pspec);

	pspec = g_param_spec_uint ("port",
	                           "accept port",
	                           "The port to use to listen for peer connections on.", 
	                           0, G_MAXUSHORT, 6881,
	                           G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_PORT, pspec);

	pspec = g_param_spec_string ("peer-id",
	                             "peer id",
	                             "The peer id that all torrents should use.",
	                             "-TXXTYY-INVALIDPEERI",
	                             G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_PEER_ID, pspec);

	pspec = g_param_spec_string ("private-dir",
	                             "private directory",
	                             "The directory in which to store preferences, .torrent files, and fast resume data.",
	                             "~/.bittorque",
	                             G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_property (gclass, PROP_PRIVATE_DIR, pspec);

	closure = g_cclosure_new (G_CALLBACK (bt_manager_new_connection_callback), NULL, NULL);

	params[0] = G_TYPE_POINTER;

	signals[SIGNAL_NEW_CONNECTION] =
		g_signal_newv ("new-connection",
		               BT_TYPE_MANAGER,
		               G_SIGNAL_RUN_LAST,
		               closure, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, params);
}
