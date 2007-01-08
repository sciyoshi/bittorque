/**
 * bittorque-plugin-info.c
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

#include "bittorque-plugin-info.h"

G_DEFINE_TYPE (BtPluginInfo, bt_plugin_info, G_TYPE_TYPE_MODULE)

enum {
	PROP_0,
	PROP_FILENAME
};

static gboolean
bt_plugin_info_load (GTypeModule *module G_GNUC_UNUSED)
{
	/* TODO */
	return TRUE;
}

static void
bt_plugin_info_unload (GTypeModule *module)
{
	BtPluginInfo *self = BT_PLUGIN_INFO (module);
	self->unload (self);
	g_module_close (self->module);
	self->module = NULL;
	self->load = NULL;
	self->unload = NULL;
}

static void
bt_plugin_info_init (BtPluginInfo *info)
{
	info->filename = NULL;
	info->module = NULL;
	info->load = NULL;
	info->unload = NULL;
}

static void
bt_plugin_info_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec)
{
	BtPluginInfo *self = BT_PLUGIN_INFO (object);

	switch (property) {
	case PROP_FILENAME:
		g_free (self->filename);
		self->filename = g_value_dup_string (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
		break;
	}
}

static void
bt_plugin_info_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec)
{
	BtPluginInfo *self = BT_PLUGIN_INFO (object);

	switch (property) {
	case PROP_FILENAME:
		g_value_set_string (value, self->filename);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
		break;
	}
}

static void
bt_plugin_info_dispose (GObject *info)
{
	BtPluginInfo *self G_GNUC_UNUSED = BT_PLUGIN_INFO (info);

	((GObjectClass *) bt_plugin_info_parent_class)->dispose (info);
}

static void
bt_plugin_info_finalize (GObject *info)
{
	BtPluginInfo *self G_GNUC_UNUSED = BT_PLUGIN_INFO (info);

	((GObjectClass *) bt_plugin_info_parent_class)->finalize (info);
}

static void
bt_plugin_info_class_init (BtPluginInfoClass *klass)
{
	GObjectClass *gclass;
	GTypeModuleClass *mclass;
	GParamSpec *pspec;

	gclass = G_OBJECT_CLASS (klass);
	mclass = G_TYPE_MODULE_CLASS (klass);

	gclass->set_property = bt_plugin_info_set_property;
	gclass->get_property = bt_plugin_info_get_property;
	gclass->finalize = bt_plugin_info_finalize;
	gclass->dispose = bt_plugin_info_dispose;
	
	mclass->load = bt_plugin_info_load;
	mclass->unload = bt_plugin_info_unload;

	pspec = g_param_spec_string ("filename",
	                             "module filename",
	                             "The filename of this module",
	                             "",
	                             G_PARAM_READWRITE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME | G_PARAM_CONSTRUCT_ONLY);

	g_object_class_install_property (gclass, PROP_FILENAME, pspec);
}

BtPluginInfo *
bt_plugin_info_new (const gchar *filename)
{
	g_return_val_if_fail (filename != NULL, NULL);
	
	return BT_PLUGIN_INFO (g_object_new (BT_TYPE_PLUGIN_INFO, "filename", filename, NULL));
}
