/*
 * bittorque.h
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef __BITTORQUE_H__
#define __BITTORQUE_H__

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include "../bittorque/bt-manager.h"
#include "../bittorque/bt-torrent.h"

G_BEGIN_DECLS

typedef struct {
	GtkWidget     *window;
	
	/* stuff for the open dialog */
	GtkWidget     *open_dialog;
	GtkWidget     *open_dialog_torrents_treeview;
	GtkWidget     *open_dialog_priority_combobox;
	GtkWidget     *open_dialog_location_chooser;
	GtkWidget     *open_dialog_super_seeding_checkbox;
	
	
	GtkStatusIcon *icon;
	BtManager     *manager;
	
} BittorqueApp;

extern BittorqueApp app;

G_END_DECLS

#endif
