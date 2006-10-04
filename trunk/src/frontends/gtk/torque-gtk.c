#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>

#include "../../libtorque/torque.h"

#include "torque-gtk.h"

TorqueApp *app;


/**
 * Callback functions
 */

void
create_torrent_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	return;
}

void
open_torrent_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	return;
}

void
quit_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gtk_main_quit ();
}

void
create_torrent_toolbutton_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	GError *error = NULL;
	TorqueTorrent *torrent = torque_torrent_new ("/home/sciyoshi/Build/test.torrent", &error);

	if (!torrent) {
		g_warning (error->message);
		return;
	}
	gchar *name, *info_hash, *peer_id;
	g_object_get (torrent, "name", &name, "info-hash", &info_hash, "peer-id", &peer_id, NULL);
	g_print ("name: %s\ninfohash: %s\npeerid: %s\n", name, info_hash, peer_id);
	g_free (name);
	g_free (info_hash);
	g_free (peer_id);
	g_object_unref (torrent);
	return;
}

void
open_torrent_cell_data_function (GtkTreeViewColumn *col G_GNUC_UNUSED, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data G_GNUC_UNUSED)
{
	TorqueTorrent *torrent;

	gtk_tree_model_get (model, iter, 0, &torrent, -1);

	g_object_set (renderer, "text", torque_torrent_get_name (torrent), NULL);
}

void
open_torrent_toolbutton_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	GtkWidget *dialog, *button;
	GtkTreeView *treeview;
	GtkListStore *model;
	gint result;

	dialog = glade_xml_get_widget (app->xml, "open_dialog");
	button = glade_xml_get_widget (app->xml, "open_dialog_remove_button");
	treeview = GTK_TREE_VIEW (glade_xml_get_widget (app->xml, "open_dialog_torrents_treeview"));

	if ((model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (treeview)))) == NULL) {
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;

		model = gtk_list_store_new (1, TORQUE_TYPE_TORRENT);
		gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (model));

		column = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_title (column, "Name");
		gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

		renderer = gtk_cell_renderer_text_new ();
		gtk_tree_view_column_pack_start (column, renderer, TRUE);

		gtk_tree_view_column_set_cell_data_func (column, renderer, open_torrent_cell_data_function, NULL, NULL);
	}

	gtk_widget_set_sensitive (button, FALSE);

	result = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_hide (dialog);

	GtkTreeIter iter;

	while (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter))
		gtk_list_store_remove (model, &iter);

	return;
}

void
open_dialog_remove_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{

}

void
open_dialog_add_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	TorqueTorrent *torrent = torque_torrent_new ("/home/sciyoshi/Build/test.torrent", NULL);
	GtkTreeView *view = GTK_TREE_VIEW (glade_xml_get_widget (app->xml, "open_dialog_torrents_treeview"));
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	GtkTreeIter iter;
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, torrent, -1);
	g_object_unref (torrent);
}


/**
 * GConf functions
 */

static void
key_changed (GConfClient *client G_GNUC_UNUSED, const gchar *key, GConfValue *value, gpointer data G_GNUC_UNUSED)
{
	if (value->type == GCONF_VALUE_STRING)
		g_print ("key %s changed to %s\n", key, gconf_value_get_string (value));

	return;
}


/**
 * Log handler
 */

static void
log_handler (const gchar *domain, GLogLevelFlags flags, const gchar *message, gpointer data)
{
	g_log_default_handler (domain, flags, message, data);
}

/**
 * Main entry point
 */

int
main (int argc, char *argv[])
{
	GError *error;

	g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL, log_handler, NULL);

	g_debug ("initializing GTK");
	gtk_init (&argc, &argv);

	app = g_malloc0 (sizeof (TorqueApp));

	g_debug ("looking for torque.glade");
	if (g_file_test ("torque.glade", G_FILE_TEST_EXISTS)) {
		app->xml = glade_xml_new ("torque.glade", NULL, NULL);
		g_debug ("found torque.glade in current directory");
	} else if (g_file_test (DATADIR "/torque.glade", G_FILE_TEST_EXISTS)) {
		app->xml = glade_xml_new (DATADIR "/torque.glade", NULL, NULL);
		g_debug ("found " DATADIR "/torque.glade");
	} else {
		g_error ("couldn't find torque.glade");
		return 1;
	}

	g_debug ("connecting to GConf");
	app->gconf = gconf_client_get_default ();

	error = NULL;

	g_debug ("adding GConf dir /apps/torque");
	gconf_client_add_dir (app->gconf, "/apps/torque", GCONF_CLIENT_PRELOAD_NONE, &error);

	if (error) {
		g_warning ("unable to watch GConf directory");
		g_clear_error (&error);
	}

	g_signal_connect (app->gconf, "value_changed", (GCallback) key_changed, &app);

	glade_xml_signal_autoconnect (app->xml);

	gtk_widget_show (glade_xml_get_widget (app->xml, "window"));

	g_debug ("done initializing, starting main loop");
	gtk_main ();

	g_object_unref (app->gconf);
	g_free (app);

	return 0;
}
