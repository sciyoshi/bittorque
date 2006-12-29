/**
 * bittorque-plugin-info.h
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

#ifndef __BITTORQUE_PLUGIN_INFO_H__
#define __BITTORQUE_PLUGIN_INFO_H__

#include <glib-object.h>
#include <gmodule.h>

#define BT_TYPE_PLUGIN_INFO (bt_plugin_info_get_type ())
#define BT_PLUGIN_INFO(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PLUGIN_INFO, BtPluginInfo))

G_BEGIN_DECLS

typedef struct _BtPluginInfo BtPluginInfo;

struct _BtPluginInfo {
	GTypeModule parent;
	
	gchar   *filename;
	GModule *module;
	
	gboolean (*load) (BtPluginInfo *info);
	gboolean (*unload) (BtPluginInfo *info);
};

typedef struct {
	GTypeModuleClass parent;
} BtPluginInfoClass;

GType         bt_plugin_info_get_type ();

BtPluginInfo *bt_plugin_info_new (const gchar *filename);

G_END_DECLS

#endif
