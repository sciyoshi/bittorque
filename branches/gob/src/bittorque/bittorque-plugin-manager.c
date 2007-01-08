/**
 * bittorque-plugin-manager.c
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

#include "bittorque-plugin-manager.h"
#include "bittorque-plugin-info.h"

G_DEFINE_TYPE (BtPluginManager, bt_plugin_manager, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_PATH
};

static gboolean
bt_plugin_manager_find_plugins (BtPluginManager *self)
{
	GDir *dir;
	const gchar *name;
	GError *error = NULL;
	
	if (!(dir = g_dir_open (self->path, 0, &error))) {
		g_warning ("could not open plugin directory: %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}
	
	while ((name = g_dir_read_name (dir))) {
		gchar *path;
		GTypeModule *module;
		path = g_build_filename (self->path, name, NULL);
		module = G_TYPE_MODULE (bt_plugin_info_new (path));
		g_free (path);
		/* TODO: more checks */
		g_type_module_use (module);
		g_type_module_unuse (module);
		self->modules = g_list_prepend (self->modules, module);
	}
	
	g_dir_close (dir);
	
	return TRUE;
}

static GObject *
bt_plugin_manager_constructor (GType type, guint num, GObjectConstructParam *properties)
{
	GObject *object;
	BtPluginManager *self;

	object = G_OBJECT_CLASS (bt_plugin_manager_parent_class)->constructor (type, num, properties);
	self = BT_PLUGIN_MANAGER (object);

	if (self->path)
		bt_plugin_manager_find_plugins (self);

	return object;
}

static void
bt_plugin_manager_init (BtPluginManager *manager)
{
	manager->path = NULL;
	manager->modules = NULL;
}

static void
bt_plugin_manager_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec)
{
	BtPluginManager *self = BT_PLUGIN_MANAGER (object);

	switch (property) {
	case PROP_PATH:
		g_free (self->path);
		self->path = g_value_dup_string (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
		break;
	}
}

static void
bt_plugin_manager_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec)
{
	BtPluginManager *self = BT_PLUGIN_MANAGER (object);

	switch (property) {
	case PROP_PATH:
		g_value_set_string (value, self->path);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
		break;
	}
}

static void
bt_plugin_manager_dispose (GObject *manager)
{
	BtPluginManager *self G_GNUC_UNUSED = BT_PLUGIN_MANAGER (manager);

	((GObjectClass *) bt_plugin_manager_parent_class)->dispose (manager);
}

static void
bt_plugin_manager_finalize (GObject *manager)
{
	BtPluginManager *self G_GNUC_UNUSED = BT_PLUGIN_MANAGER (manager);

	((GObjectClass *) bt_plugin_manager_parent_class)->finalize (manager);
}

static void
bt_plugin_manager_class_init (BtPluginManagerClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;

	gclass = G_OBJECT_CLASS (klass);

	gclass->set_property = bt_plugin_manager_set_property;
	gclass->get_property = bt_plugin_manager_get_property;
	gclass->constructor = bt_plugin_manager_constructor;
	gclass->finalize = bt_plugin_manager_finalize;
	gclass->dispose = bt_plugin_manager_dispose;

	pspec = g_param_spec_string ("path",
	                             "module path",
	                             "Directory in which to look for plugins",
	                             "",
	                             G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_property (gclass, PROP_PATH, pspec);
}
