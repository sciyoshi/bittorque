/**
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
 * Foundation, Inc., 51 Franklin St,, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include "bittorque.h"
#include "gnet.h"

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

BT_EXPORT void
open_dialog_super_seeding_checkbox_toggled (GtkCheckButton *button, gpointer data G_GNUC_UNUSED)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (app.open_dialog_torrents_treeview)), &model, &iter))
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, COLUMN_SUPER_SEEDING, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)), -1);
}

BT_EXPORT void
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

BT_EXPORT void
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
	GtkTreeModel *files_model;
	gboolean super_seeding;
	gchar *download_location;
	BtTorrentPriority priority;
	BtTorrent *torrent;

	files_model = gtk_tree_view_get_model (GTK_TREE_VIEW (app.open_dialog_files_treeview));

	g_return_if_fail (files_model != NULL);

	gtk_list_store_clear (GTK_LIST_STORE (files_model));

	/* if selection is NULL, we were called manually */
	if (selection != NULL && gtk_tree_selection_get_selected (selection, &model, &iter)) {
		GtkTreeIter files_iter;
		guint i;

		gtk_widget_set_sensitive (app.open_dialog_location_chooser, TRUE);
		gtk_widget_set_sensitive (app.open_dialog_priority_combobox, TRUE);
		gtk_widget_set_sensitive (app.open_dialog_super_seeding_checkbox, TRUE);
		gtk_widget_set_sensitive (app.open_dialog_remove_button, TRUE);
		gtk_tree_model_get (model, &iter, COLUMN_TORRENT, &torrent, COLUMN_LOCATION, &download_location, COLUMN_PRIORITY, &priority, COLUMN_SUPER_SEEDING, &super_seeding, -1);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (app.open_dialog_super_seeding_checkbox), super_seeding);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (app.open_dialog_location_chooser), download_location);
		gtk_combo_box_set_active (GTK_COMBO_BOX (app.open_dialog_priority_combobox), priority);

		for (i = 0; i < torrent->files->len; i++) {
			gtk_list_store_append (GTK_LIST_STORE (files_model), &files_iter);
			gtk_list_store_set (GTK_LIST_STORE (files_model), &files_iter, 0, &g_array_index (torrent->files, BtTorrentFile, i), -1);
		}

		g_free (download_location);
		g_object_unref (torrent);
	} else {
		gtk_widget_set_sensitive (app.open_dialog_location_chooser, FALSE);
		gtk_widget_set_sensitive (app.open_dialog_priority_combobox, FALSE);
		gtk_widget_set_sensitive (app.open_dialog_super_seeding_checkbox, FALSE);
		gtk_widget_set_sensitive (app.open_dialog_remove_button, FALSE);
		gtk_combo_box_set_active (GTK_COMBO_BOX (app.open_dialog_priority_combobox), -1);
		gtk_list_store_clear (GTK_LIST_STORE (files_model));
	}

	return;
}

static void
open_dialog_files_name_cell_renderer_func (GtkTreeViewColumn *col G_GNUC_UNUSED, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data G_GNUC_UNUSED)
{
	BtTorrentFile *file;

	gtk_tree_model_get (model, iter, 0, &file, -1);

	g_object_set (cell, "text", file->name, NULL);

	g_boxed_free (BT_TYPE_TORRENT_FILE, (gpointer) file);
}

static void
open_dialog_files_download_cell_renderer_func (GtkTreeViewColumn *col G_GNUC_UNUSED, GtkCellRenderer *cell, GtkTreeModel *model, GtkTreeIter *iter, gpointer data G_GNUC_UNUSED)
{
	BtTorrentFile *file;

	gtk_tree_model_get (model, iter, 0, &file, -1);

	g_object_set (cell, "active", file->download, NULL);

	g_boxed_free (BT_TYPE_TORRENT_FILE, (gpointer) file);
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

	torrent = bt_torrent_new (app.manager, filename);

	g_return_val_if_fail (BT_IS_TORRENT (torrent), FALSE);

	gtk_list_store_append (list, &iter);
	gtk_list_store_set (list, &iter, COLUMN_TORRENT, torrent, COLUMN_LOCATION, g_get_home_dir (), COLUMN_PRIORITY, BT_TORRENT_PRIORITY_NORMAL, COLUMN_SUPER_SEEDING, FALSE, -1);
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

BT_EXPORT void
open_dialog_add_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gchar *filename = open_dialog_show_add_file_dialog ();

	if (filename)
		open_dialog_add_file (filename);
}

BT_EXPORT void
open_dialog_remove_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (app.open_dialog_torrents_treeview));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		g_signal_handlers_block_by_func (selection, open_dialog_selection_changed, NULL);

		valid = gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

		g_signal_handlers_unblock_by_func (selection, open_dialog_selection_changed, NULL);

		if (valid)
			gtk_tree_selection_select_iter (selection, &iter);
		else
			open_dialog_selection_changed (NULL, NULL);
	}
}

static void
open_dialog_run (gchar *filename)
{
	GtkWidget *dialog;
	GtkWidget *priority;
	GtkListStore *list;
	GtkListStore *files;
	GtkWidget *view;
	GtkWidget *files_view;
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;
	GtkTreeIter iter;
	BtTorrent *torrent;
	gchar *location;
	gboolean valid;
	gint response;

	view = app.open_dialog_torrents_treeview;
	files_view = app.open_dialog_files_treeview;

	if (gtk_tree_view_get_model (GTK_TREE_VIEW (view)) == NULL) {
		list = gtk_list_store_new (COLUMN_TOTAL, BT_TYPE_TORRENT, G_TYPE_STRING, BT_TYPE_TORRENT_PRIORITY, G_TYPE_BOOLEAN);
		gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (list));
		g_object_unref (list);

		col = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_title (col, _("Name"));

		cell = gtk_cell_renderer_text_new ();
		gtk_tree_view_column_pack_start (col, cell, TRUE);
		gtk_tree_view_column_set_cell_data_func (col, cell, open_dialog_cell_renderer_func, NULL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

		g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)), "changed", G_CALLBACK (open_dialog_selection_changed), NULL);
	} else {
		list = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
	}

	if (gtk_tree_view_get_model (GTK_TREE_VIEW (files_view)) == NULL) {
		files = gtk_list_store_new (1, BT_TYPE_TORRENT_FILE);
		gtk_tree_view_set_model (GTK_TREE_VIEW (files_view), GTK_TREE_MODEL (files));
		g_object_unref (files);

		col = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_title (col, _("Download"));
		cell = gtk_cell_renderer_toggle_new ();
		gtk_tree_view_column_pack_start (col, cell, TRUE);
		gtk_tree_view_column_set_cell_data_func (col, cell, open_dialog_files_download_cell_renderer_func, NULL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (files_view), col);

		col = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_title (col, _("File"));
		cell = gtk_cell_renderer_text_new ();
		gtk_tree_view_column_pack_start (col, cell, TRUE);
		gtk_tree_view_column_set_cell_data_func (col, cell, open_dialog_files_name_cell_renderer_func, NULL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (files_view), col);
	} else {
		files = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (files_view)));
	}

	if (filename)
		open_dialog_add_file (filename);
	else
		open_dialog_selection_changed (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)), NULL);

	dialog = app.open_dialog;
	priority = app.open_dialog_priority_combobox;

	gtk_combo_box_set_active (GTK_COMBO_BOX (priority), 1);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (response) {
		case GTK_RESPONSE_REJECT:
			break;

		case GTK_RESPONSE_ACCEPT:
			valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list), &iter);

			while (valid) {
				gtk_tree_model_get (GTK_TREE_MODEL (list), &iter, COLUMN_TORRENT, &torrent, COLUMN_LOCATION, &location, -1);
				bt_torrent_set_location (torrent, location);
				g_free (location);
				bt_torrent_add_peer (torrent, bt_peer_new (app.manager, torrent, NULL, gnet_inetaddr_new ("127.0.0.1", 6838)));
				bt_manager_add_torrent (app.manager, torrent);
				bt_torrent_start_downloading (torrent, NULL);
				g_object_unref (torrent);
				valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (list), &iter);
			};

			break;

		default:
			break;
	}

	gtk_widget_hide (dialog);

	gtk_list_store_clear (list);
	gtk_list_store_clear (files);
}

static void
open_dialog_show_add_dialog_and_run ()
{
	gchar *filename = open_dialog_show_add_file_dialog ();

	if (filename != NULL)
		open_dialog_run (filename);
}

BT_EXPORT void
open_torrent_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	open_dialog_show_add_dialog_and_run ();
}

BT_EXPORT void
open_torrent_toolbutton_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	open_dialog_show_add_dialog_and_run ();
}
