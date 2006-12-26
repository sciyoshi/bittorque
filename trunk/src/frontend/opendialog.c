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

#include "bittorque.h"

enum {
	COLUMN_TORRENT,
	COLUMN_LOCATION,
	COLUMN_PRIORITY,
	COLUMN_SUPER_SEEDING,
	COLUMN_TOTAL
};

static void
open_dialog_cell_renderer_func (GtkTreeViewColumn *col G_GNUC_UNUSED, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data G_GNUC_UNUSED)
{
	BtTorrent *torrent;

	gtk_tree_model_get (model, iter, COLUMN_TORRENT, &torrent, -1);

	g_object_set (cell, "text", torrent->name, NULL);

	g_object_unref (torrent);
}

void
open_dialog_super_seeding_checkbox_toggled (GtkCheckButton *button, gpointer data G_GNUC_UNUSED)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (app.open_dialog_torrents_treeview)), &model, &iter))
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_SUPER_SEEDING, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)), -1);
}

void
open_dialog_location_chooser_current_folder_changed (GtkFileChooser *chooser, gpointer data G_GNUC_UNUSED)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (app.open_dialog_torrents_treeview)), &model, &iter)) {
		gchar *filename = gtk_file_chooser_get_filename (chooser);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_LOCATION, filename, -1);
		g_free (filename);
	}
}

void
open_dialog_priority_combobox_changed (GtkComboBox *combobox, gpointer data G_GNUC_UNUSED)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (app.open_dialog_torrents_treeview)), &model, &iter))
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_PRIORITY, gtk_combo_box_get_active (combobox), -1);
}

static void
open_dialog_selection_changed (GtkTreeSelection *selection, gpointer data G_GNUC_UNUSED)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean super_seeding;
	gchar *download_location;
	BtTorrentPriority priority;
	
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_widget_set_sensitive (app.open_dialog_location_chooser, TRUE);
		gtk_widget_set_sensitive (app.open_dialog_priority_combobox, TRUE);
		gtk_widget_set_sensitive (app.open_dialog_super_seeding_checkbox, TRUE);
		gtk_tree_model_get (model, &iter, COLUMN_LOCATION, &download_location, COLUMN_PRIORITY, &priority, COLUMN_SUPER_SEEDING, &super_seeding, -1);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app.open_dialog_super_seeding_checkbox), super_seeding);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (app.open_dialog_location_chooser), download_location);
		gtk_combo_box_set_active (GTK_COMBO_BOX (app.open_dialog_priority_combobox), priority);
		g_free (download_location);
	} else {
		gtk_widget_set_sensitive (app.open_dialog_location_chooser, FALSE);
		gtk_widget_set_sensitive (app.open_dialog_priority_combobox, FALSE);
		gtk_widget_set_sensitive (app.open_dialog_super_seeding_checkbox, FALSE);
		gtk_combo_box_set_active (GTK_COMBO_BOX (app.open_dialog_priority_combobox), -1);
	}
	return;
}

static gboolean
open_dialog_add_file (gchar *filename)
{
	GtkListStore *list;
	GtkTreeIter iter;
	GtkWidget *view;
	BtTorrent *torrent;
	GError *error;

	g_return_val_if_fail (filename != NULL, FALSE);

	view = app.open_dialog_torrents_treeview;

	list = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

	error = NULL;

	if (!(torrent = bt_torrent_new (app.manager, filename, &error))) {
		g_warning ("could not add torrent: %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	gtk_list_store_append (list, &iter);
	gtk_list_store_set (list, &iter, COLUMN_TORRENT, torrent, COLUMN_LOCATION, "/home/sciyoshi", COLUMN_PRIORITY, BT_TORRENT_PRIORITY_NORMAL, COLUMN_SUPER_SEEDING, FALSE, -1);
	g_object_unref (torrent);
	
	gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)), &iter);

	return TRUE;
}

static gchar *
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

static void
open_dialog_run (gchar *filename)
{
	GtkWidget *dialog;
	GtkWidget *priority;
	GtkListStore *list;
	GtkWidget *view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;

	view = app.open_dialog_torrents_treeview;

	list = gtk_list_store_new (COLUMN_TOTAL, BT_TYPE_TORRENT, G_TYPE_STRING, BT_TYPE_TORRENT_PRIORITY, G_TYPE_BOOLEAN);
	
	gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (list));

	g_object_unref (list);

	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)), "changed", G_CALLBACK (open_dialog_selection_changed), NULL);

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Name"));
	
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_cell_data_func (col, cell, open_dialog_cell_renderer_func, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

	if (filename)
		open_dialog_add_file (filename);

	dialog = app.open_dialog;
	priority = app.open_dialog_priority_combobox;

	gtk_combo_box_set_active (GTK_COMBO_BOX (priority), 1);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_hide (dialog);
}

static void
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
