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

G_DEFINE_TYPE (BtPluginManager, bt_plugin_manager, G_TYPE_OBJECT)

enum {
	PROP_0,
	PROP_PATH
};

void
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

void
bt_plugin_manager_class_init (BtPluginManagerClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;

	gclass = G_OBJECT_CLASS (klass);

	gclass->set_property = bt_plugin_manager_set_property;
	gclass->get_property = bt_plugin_manager_get_property;
	gclass->finalize = bt_plugin_manager_finalize;
	gclass->dispose = bt_plugin_manager_dispose;

	pspec = g_param_spec_string ("path",
	                             "module search path",
	                             "List of paths in which to look for plugins",
	                             "",
	                             G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_PATH, pspec);
}
