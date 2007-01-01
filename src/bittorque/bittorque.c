/**
 * bittorque.c
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

BtApp app;

/**
 * Callback functions
 */

BT_EXPORT void
about_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gtk_dialog_run (GTK_DIALOG (app.about_dialog));
	
	gtk_widget_hide (app.about_dialog);
}

BT_EXPORT void
create_torrent_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	return;
}

BT_EXPORT void
quit_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gtk_main_quit ();
}

BT_EXPORT void
preferences_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gtk_dialog_run (GTK_DIALOG (app.preferences_dialog));
	gtk_widget_hide (app.preferences_dialog);
}

BT_EXPORT void
plugins_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gtk_dialog_run (GTK_DIALOG (app.plugins_dialog));
	gtk_widget_hide (app.plugins_dialog);
}

BT_EXPORT void
create_torrent_toolbutton_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
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
 * Loads widgets from the glade file
 */

static void
load_widgets (GladeXML *xml)
{
	app.window = glade_xml_get_widget (xml, "window");

	app.about_dialog = glade_xml_get_widget (xml, "about_dialog");
	app.preferences_dialog = glade_xml_get_widget (xml, "preferences_dialog");
	app.plugins_dialog = glade_xml_get_widget (xml, "plugins_dialog");
	
	gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (app.about_dialog), "BitTorque");
	
	app.open_dialog = glade_xml_get_widget (xml, "open_dialog");
	app.open_dialog_torrents_treeview = glade_xml_get_widget (xml, "open_dialog_torrents_treeview");
	app.open_dialog_priority_combobox = glade_xml_get_widget (xml, "open_dialog_priority_combobox");
	app.open_dialog_location_chooser = glade_xml_get_widget (xml, "open_dialog_location_chooser");
	app.open_dialog_super_seeding_checkbox = glade_xml_get_widget (xml, "open_dialog_super_seeding_checkbox");
	app.open_dialog_remove_button = glade_xml_get_widget (xml, "open_dialog_remove_button");
	app.open_dialog_files_treeview = glade_xml_get_widget (xml, "open_dialog_files_treeview");
}

/**
 * Main entry point
 */

int
main (int argc, char *argv[])
{
	GladeXML *xml = NULL;
	GError *error = NULL;

	/* register our own log handler for all logs */
	g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL, log_handler, NULL);

	gnet_init ();

	gtk_init (&argc, &argv);

	/* look for the glade file, first in BT_DATA_DIR, then in current directory */
	if (g_file_test (BT_DATA_DIR "/bittorque.glade", G_FILE_TEST_EXISTS))
		xml = glade_xml_new (BT_DATA_DIR "/bittorque.glade", NULL, NULL);
	else if (g_file_test ("bittorque.glade", G_FILE_TEST_EXISTS))
		xml = glade_xml_new ("bittorque.glade", NULL, NULL);

	if (!xml) {
		g_error ("couldn't find bittorque.glade, check your installation");
		return 1;
	}

	/* create the BtManager */
	app.manager = bt_manager_new ();

	if (!bt_manager_set_port (app.manager, 32459, &error)) {
		g_error ("couldn't set port: %s", error->message);
		return 1;
	}

	/* start accepting connections */
	if (!bt_manager_accept_start (app.manager, &error)) {
		g_error ("couldn't start manager: %s", error->message);
		return 1;
	}

	/* autoconnect all our callbacks from the glade file (--export-dynamic) */
	glade_xml_signal_autoconnect (xml);

	/* load all the widgets we'll need beforehand... */
	load_widgets (xml);
	
	/* ...so that we can unload the glade file */
	g_object_unref (xml);

	/* create the status icon */
	app.icon = gtk_status_icon_new_from_file (BT_DATA_DIR "/bittorque.png");
	gtk_status_icon_set_tooltip (app.icon, "BitTorque");
	gtk_status_icon_set_visible (app.icon, TRUE);

	/* show the main window */
	gtk_widget_show (app.window);

	/* run the main loop */
	gtk_main ();

	/* stop accepting */
	bt_manager_accept_stop (app.manager);

	return 0;
}
