/**
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA, 02110-1301, USA
 */

#ifndef __BITTORQUE_H__
#define __BITTORQUE_H__

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "bt-manager.h"
#include "bt-torrent.h"

G_BEGIN_DECLS

/* this is for --export-dynamic and glade's autoconnect */
#ifdef G_OS_WIN32
# define BT_EXPORT __declspec (dllexport)
#else
# define BT_EXPORT
#endif

typedef struct {
	/* the main window */
	GtkWidget     *window;

	GtkWidget     *about_dialog;
	GtkWidget     *preferences_dialog;
	GtkWidget     *plugins_dialog;

	/* stuff for the open dialog */
	GtkWidget     *open_dialog;
	GtkWidget     *open_dialog_torrents_treeview;
	GtkWidget     *open_dialog_remove_button;
	GtkWidget     *open_dialog_priority_combobox;
	GtkWidget     *open_dialog_location_chooser;
	GtkWidget     *open_dialog_super_seeding_checkbox;
	GtkWidget     *open_dialog_files_treeview;

	/* statusbar icon */
	GdkPixbuf     *icon;
	GtkStatusIcon *status_icon;

	/* torrent manager */
	BtManager     *manager;

} BtApp;

extern BtApp app;

G_END_DECLS

#endif
