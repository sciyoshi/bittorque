#include <glib.h>
#include <glib-object.h>

#include "torque-torrent.h"
#include "torque-manager.h"

typedef enum {
	PROPERTY_PORT = 1,
} TorqueManagerProperty;

struct _TorqueManager {
	GObject parent;

	GHashTable *torrents;

	gushort port;
};

struct _TorqueManagerClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (TorqueManager, torque_manager, G_TYPE_OBJECT)

static void
torque_manager_set_property (GObject *obj, guint property, const GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	TorqueManager *self = TORQUE_MANAGER (obj);

	switch (property) {
	case PROPERTY_PORT:
		self->port = (gushort) g_value_get_uint (value);
		break;
	}
}

static void
torque_manager_get_property (GObject *obj, guint property, GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	TorqueManager *self = TORQUE_MANAGER (obj);

	switch (property) {
	case PROPERTY_PORT:
		g_value_set_uint (value, self->port);
		break;
	}
}

static void
torque_manager_init (TorqueManager *self G_GNUC_UNUSED)
{
	return;
}

static void
torque_manager_class_init (TorqueManagerClass *klass)
{
	GParamSpec *param;

	GObjectClass *g_class = G_OBJECT_CLASS (klass);

	g_class->get_property = torque_manager_get_property;
	g_class->set_property = torque_manager_set_property;

	param = g_param_spec_uint ("port", "port number", "The port for this manager to listen on. Incoming connections will be demuxed to the appropriate torrents.", 0, G_MAXUSHORT, 0, G_PARAM_READWRITE);
	g_object_class_install_property (g_class, PROPERTY_PORT, param);

	return;
}

TorqueManager *
torque_manager_new ()
{
	TorqueManager *manager = TORQUE_MANAGER (g_object_new (TORQUE_TYPE_MANAGER, NULL));

	manager->torrents = g_hash_table_new (g_str_hash, g_str_equal);

	return manager;
}

gboolean
torque_manager_add_torrent (TorqueManager *manager, TorqueTorrent *torrent, GError **error G_GNUC_UNUSED)
{
	g_return_val_if_fail (manager && torrent, FALSE);

	g_hash_table_insert (manager->torrents, (gpointer) torque_torrent_get_info_hash (torrent), g_object_ref (torrent));

	return TRUE;
}

gboolean
torque_manager_remove_torrent (TorqueManager *manager, TorqueTorrent *torrent, GError **error G_GNUC_UNUSED)
{
	g_return_val_if_fail (manager && torrent, FALSE);

	const gchar *hash = torque_torrent_get_info_hash (torrent);

	TorqueTorrent *found = TORQUE_TORRENT (g_hash_table_lookup (manager->torrents, (gpointer) hash));

	if (!found)
		return FALSE;

	g_object_unref (found);
	g_hash_table_remove (manager->torrents, (gpointer) hash);

	return TRUE;
}
