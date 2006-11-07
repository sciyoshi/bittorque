#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gnet.h>

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
	GtkTreeViewColumn *col;
	GtkCellRenderer *cell;
	BtTorrent *torrent;
	GError *error;

	view = glade_xml_get_widget (app.xml, "open_dialog_torrents_treeview");

	list = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));

	if (!list) {
		list = gtk_list_store_new (1, BT_TYPE_TORRENT);
		gtk_tree_view_set_model (GTK_TREE_VIEW (view), GTK_TREE_MODEL (list));

		col = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_title (col, "Name");
		cell = gtk_cell_renderer_text_new ();
		gtk_tree_view_column_pack_start (col, cell, TRUE);
		gtk_tree_view_column_set_cell_data_func (col, cell, cell_renderer_func, NULL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);
	}

	error = NULL;

	if (!(torrent = bt_torrent_new (app.manager, filename, &error))) {
		g_warning ("could not add torrent: %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	bt_manager_add_torrent (app.manager, torrent);

	bt_torrent_start_downloading (torrent);

	bt_peer_connect (bt_peer_new (app.manager, torrent, NULL, gnet_inetaddr_new_nonblock ("127.0.0.1", 6552)));

	gtk_list_store_append (list, &iter);
	gtk_list_store_set (list, &iter, 0, torrent, -1);
	g_object_unref (torrent);

	return TRUE;
}

void
open_dialog_add_file_dialog ()
{
	GtkWidget *dialog;
	gint response;
	gchar *filename;

	dialog = gtk_file_chooser_dialog_new ("Select Torrent...", GTK_WINDOW (app.window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		open_dialog_add_file (filename);
		g_free (filename);
	}

	gtk_widget_destroy (dialog);

}

void
open_dialog_add_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	open_dialog_add_file_dialog ();
}


void
open_dialog_remove_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{

}

void
open_dialog_run_with_file (gchar *filename)
{
	GtkWidget *dialog;
	GtkWidget *priority;

	if (filename)
		open_dialog_add_file (filename);

	dialog = glade_xml_get_widget (app.xml, "open_dialog");
	priority = glade_xml_get_widget (app.xml, "open_dialog_priority_combobox");

	gtk_combo_box_set_active (GTK_COMBO_BOX (priority), 1);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_hide (dialog);
}

void
open_dialog_run ()
{
	open_dialog_run_with_file (NULL);
}

void
open_torrent_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	open_dialog_add_file_dialog ();
	open_dialog_run ();
}

void
open_torrent_toolbutton_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	open_dialog_add_file_dialog ();
	open_dialog_run ();
}
