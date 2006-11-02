#include <unistd.h>

#include "bittorque.h"

BittorqueApp app;

/**
 * Callback functions
 */

void
create_torrent_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
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
	return;
}


#ifdef USE_GCONF

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

#endif


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

	error = NULL;

	/* register our own log handler for all logs */
	g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL, log_handler, NULL);

	gnet_init ();

	gtk_init (&argc, &argv);

	/* look for the glade file, first in DATADIR, then in current directory */
	if (g_file_test (DATADIR "/bittorque.glade", G_FILE_TEST_EXISTS))
		app.xml = glade_xml_new (DATADIR "/bittorque.glade", NULL, NULL);
	else if (g_file_test ("bittorque.glade", G_FILE_TEST_EXISTS))
		app.xml = glade_xml_new ("bittorque.glade", NULL, NULL);
	else {
		g_error ("couldn't find bittorque.glade");
		return 1;
	}

#ifdef USE_GCONF
	app.gconf = gconf_client_get_default ();

	gconf_client_add_dir (app.gconf, "/apps/bittorque", GCONF_CLIENT_PRELOAD_NONE, &error);

	if (error) {
		g_warning ("unable to watch GConf directory");
		g_clear_error (&error);
	}

	g_signal_connect (app.gconf, "value_changed", (GCallback) key_changed, &app);
#endif

	/* create the BtManager */
	app.manager = bt_manager_new ();

	if (!bt_manager_set_port (app.manager, 6552, &error)) {
		g_error ("couldn't set port: %s", error->message);
		return 1;
	}

	/* start accepting connections */
	if (!bt_manager_accept_start (app.manager, &error)) {
		g_error ("couldn't start manager: %s", error->message);
		return 1;
	}

	/* autoconnect all our callbacks from the glade file. this is why we need --export-dynamic */
	glade_xml_signal_autoconnect (app.xml);

	app.window = glade_xml_get_widget (app.xml, "window");

	app.icon = gtk_status_icon_new_from_stock ("gtk-preferences");
	gtk_status_icon_set_tooltip (app.icon, "BitTorque");
	gtk_status_icon_set_visible (app.icon, TRUE);

	/* show the main window */
	gtk_widget_show (app.window);

	gtk_main ();

	/* stop accepting */
	bt_manager_accept_stop (app.manager);

#ifdef USE_GCONF
	g_object_unref (app.gconf);
#endif

	return 0;
}
