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

BtApp *app;

G_DEFINE_TYPE (BtApp, bt_app, G_TYPE_OBJECT)

/**
 * Callback functions
 */

BT_EXPORT void
about_menuitem_activated (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gtk_dialog_run (GTK_DIALOG (app->about_dialog));
	
	gtk_widget_hide (app->about_dialog);
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
create_torrent_toolbutton_clicked (GtkWidget *widget G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	return;
}

enum {
	PROP_0,
	PROP_WINDOW,
	PROP_MANAGER
};

static void
bt_app_set_property (GObject *object, guint property, const GValue *value G_GNUC_UNUSED, GParamSpec *pspec G_GNUC_UNUSED)
{
	
}

static void
bt_app_get_property (GObject *object, guint property, GValue *value G_GNUC_UNUSED, GParamSpec *pspec G_GNUC_UNUSED)
{
	BtApp *self = BT_APP (object);

	switch (property) {
	case PROP_WINDOW:
		g_value_set_object (value, self->window);
		break;

	case PROP_MANAGER:
		g_value_set_object (value, self->manager);
		break;
	}

}

static void
bt_app_finalize (GObject *app)
{
	
}

static void
bt_app_dispose (GObject *app)
{
	
}

static void
bt_app_init (BtApp *app)
{
	
}

static void
bt_app_class_init (BtAppClass *klass)
{
	GObjectClass *gclass;
	GParamSpec *pspec;

	gclass = G_OBJECT_CLASS (klass);

	gclass->set_property = bt_app_set_property;
	gclass->get_property = bt_app_get_property;
	gclass->finalize = bt_app_finalize;
	gclass->dispose = bt_app_dispose;

	pspec = g_param_spec_object ("window",
	                             "application main window",
	                             "The main application window",
	                             GTK_TYPE_WINDOW,
	                             G_PARAM_READABLE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_WINDOW, pspec);
	
	pspec = g_param_spec_object ("manager",
	                             "torrent manager",
	                             "The torrent manager",
	                             BT_TYPE_MANAGER,
	                             G_PARAM_READABLE | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME);

	g_object_class_install_property (gclass, PROP_MANAGER, pspec);

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
	app->window = glade_xml_get_widget (xml, "window");

	app->about_dialog = glade_xml_get_widget (xml, "about_dialog");
	
	gtk_about_dialog_set_name (GTK_ABOUT_DIALOG (app->about_dialog), "BitTorque");
	
	app->open_dialog = glade_xml_get_widget (xml, "open_dialog");
	app->open_dialog_torrents_treeview = glade_xml_get_widget (xml, "open_dialog_torrents_treeview");
	app->open_dialog_priority_combobox = glade_xml_get_widget (xml, "open_dialog_priority_combobox");
	app->open_dialog_location_chooser = glade_xml_get_widget (xml, "open_dialog_location_chooser");
	app->open_dialog_super_seeding_checkbox = glade_xml_get_widget (xml, "open_dialog_super_seeding_checkbox");
	app->open_dialog_remove_button = glade_xml_get_widget (xml, "open_dialog_remove_button");
	app->open_dialog_files_treeview = glade_xml_get_widget (xml, "open_dialog_files_treeview");
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

	app = BT_APP (g_object_new (BT_TYPE_APP, NULL));

	/* look for the glade file, first in DATADIR, then in current directory */
	if (g_file_test (DATADIR "/bittorque.glade", G_FILE_TEST_EXISTS))
		xml = glade_xml_new (DATADIR "/bittorque.glade", NULL, NULL);
	else if (g_file_test ("bittorque.glade", G_FILE_TEST_EXISTS))
		xml = glade_xml_new ("bittorque.glade", NULL, NULL);

	if (!xml) {
		g_error ("couldn't find bittorque.glade, check your installation");
		return 1;
	}

	/* create the BtManager */
	app->manager = bt_manager_new ();

	if (!bt_manager_set_port (app->manager, 6552, &error)) {
		g_error ("couldn't set port: %s", error->message);
		return 1;
	}

	/* start accepting connections */
	if (!bt_manager_accept_start (app->manager, &error)) {
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
	app->icon = gtk_status_icon_new_from_stock ("gtk-preferences");
	gtk_status_icon_set_tooltip (app->icon, "BitTorque");
	gtk_status_icon_set_visible (app->icon, TRUE);

	/* show the main window */
	gtk_widget_show (app->window);

	/* run the main loop */
	gtk_main ();

	/* stop accepting */
	bt_manager_accept_stop (app->manager);

	return 0;
}
