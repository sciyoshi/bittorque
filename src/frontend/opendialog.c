/*
 * opendialog.c
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

#include <glib/gi18n.h>

#include "bittorque.h"

static void
cell_renderer_func (GtkTreeViewColumn *col G_GNUC_UNUSED, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data G_GNUC_UNUSED)
{
	BtTorrent *torrent;

	gtk_tree_model_get (model, iter, 0, &torrent, -1);

	g_object_set (cell, "text", torrent->name, NULL);

	g_object_unref (torrent);
}

gboolean
open_dialog_add_file (gchar *filename)
{
	GtkListStore *list;
	GtkTreeIter iter;
	GtkWidget *view;
	BtTorrent *torrent;
	GError *error;

	g_return_val_if_fail (filename != NULL, FALSE);

	view = glade_xml_get_widget (app.xml, "open_dialog_torrents_treeview");

	list = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

	error = NULL;

	if (!(torrent = bt_torrent_new (app.manager, filename, &error))) {
		g_warning ("could not add torrent: %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	gtk_list_store_append (list, &iter);
	gtk_list_store_set (list, &iter, 0, torrent, -1);
	g_object_unref (torrent);
	
	gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)), &iter);

	return TRUE;
}

gchar *
open_dialog_show_add_file_dialog ()
{
	GtkWidget *dialog;
	GtkFileFilter *filter;
	gint response;
	gchar *filename;

	dialog = gtk_file_chooser_dialog_new (_("Select Torrent..."), GTK_WINDOW (app.window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Torrent metainfo files (*.torrent)"));
	gtk_file_filter_add_pattern (filter, "*.torrent");

	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	else
		filename = NULL;

	gtk_widget_destroy (dialog);

	return filename;
}

void
open_dialog_add_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gchar *filename = open_dialog_show_add_file_dialog ();
	
	if (filename)
		open_dialog_add_file (filename);
}

void
open_dialog_remove_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{

}

void
open_dialog_run (gchar *filename)
{
	GtkWidget *dialog;
	GtkWidget *priority;
	GtkListStore *list;
	GtkWidget *view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;

	view = glade_xml_get_widget (app.xml, "open_dialog_torrents_treeview");

	list = gtk_list_store_new (1, BT_TYPE_TORRENT);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (list));

	g_object_unref (list);

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, "Name");
	
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_cell_data_func (col, cell, cell_renderer_func, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

	if (filename)
		open_dialog_add_file (filename);

	dialog = glade_xml_get_widget (app.xml, "open_dialog");
	priority = glade_xml_get_widget (app.xml, "open_dialog_priority_combobox");

	gtk_combo_box_set_active (GTK_COMBO_BOX (priority), 1);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_hide (dialog);
}

void
open_dialog_show_add_dialog_and_run ()
{
	gchar *filename = open_dialog_show_add_file_dialog ();
	
	if (filename != NULL)
		open_dialog_run (filename);
}

void
open_torrent_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	open_dialog_show_add_dialog_and_run ();
}

void
open_torrent_toolbutton_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	open_dialog_show_add_dialog_and_run ();
}
