/**
 * bittorque-ui.c
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

#include "bittorque.h"
#include "bt-torrent.h"
#include "bt-utils.h"

void
torrents_status_cell_renderer_func (GtkTreeViewColumn *column G_GNUC_UNUSED,
                                    GtkCellRenderer   *renderer,
                                    GtkTreeModel      *model,
                                    GtkTreeIter       *iter,
                                    gpointer           data G_GNUC_UNUSED)
{
	BtTorrent *torrent;

	gtk_tree_model_get (model, iter, 0, &torrent, -1);

	g_object_set (renderer, "stock-id", "gtk-save", NULL);

	g_object_unref (G_OBJECT (torrent));
}

void
torrents_size_cell_renderer_func (GtkTreeViewColumn *column G_GNUC_UNUSED,
                                  GtkCellRenderer   *renderer,
                                  GtkTreeModel      *model,
                                  GtkTreeIter       *iter,
                                  gpointer           data G_GNUC_UNUSED)
{
	BtTorrent *torrent;
	gchar *string;

	gtk_tree_model_get (model, iter, 0, &torrent, -1);

	string = bt_size_to_string (bt_torrent_get_size (torrent));

	g_object_set (renderer, "text", string, NULL);

	g_free (string);

	g_object_unref (G_OBJECT (torrent));
}

void
torrents_name_cell_renderer_func (GtkTreeViewColumn *column G_GNUC_UNUSED,
                                  GtkCellRenderer   *renderer,
                                  GtkTreeModel      *model,
                                  GtkTreeIter       *iter,
                                  gpointer           data G_GNUC_UNUSED)
{
	BtTorrent *torrent;

	gtk_tree_model_get (model, iter, 0, &torrent, -1);

	g_object_set (renderer, "text", bt_torrent_get_name (torrent), NULL);

	g_object_unref (G_OBJECT (torrent));
}

void
bittorque_open_torrent ()
{
	GtkResponseType response;
	GtkWidget      *chooser;
	GtkFileFilter  *filter;
	BtTorrent      *torrent;
	GtkTreeIter     iter;
	GError         *error = NULL;
	gchar          *filename;
	
	chooser = gtk_file_chooser_dialog_new (
		_("Open Torrent..."),
		GTK_WINDOW (bittorque.main_window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Torrent metainfo files (*.torrent)"));
	gtk_file_filter_add_pattern (filter, "*.torrent");

	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);
	
	response = gtk_dialog_run (GTK_DIALOG (chooser));
	
	if (response == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
		
		torrent = bt_torrent_new (bittorque.manager, filename, &error);
		
		bt_manager_add_torrent (bittorque.manager, torrent);
		
		gtk_list_store_append (bittorque.torrents_list, &iter);
		
		gtk_list_store_set (bittorque.torrents_list, &iter, 0, torrent, -1);
		
		bt_torrent_start (torrent);
		
		g_object_unref (torrent);
		
		g_free (filename);
	}
	
	gtk_widget_destroy (chooser);
}

void
bittorque_open_preferences ()
{
	gtk_dialog_run (GTK_DIALOG (bittorque.preferences_dialog));
	
	gtk_widget_hide (bittorque.preferences_dialog);
}
