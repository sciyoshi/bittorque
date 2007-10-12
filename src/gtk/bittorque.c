/**
 * bittorque.c
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

#include <gnet.h>

#include "bittorque.h"
#include "bittorque-ui.h"

#ifdef BITTORQUE_EMBED_DATA
# include "bittorque-ui.h"
# include "bittorque-icon-16.h"
# include "bittorque-icon-24.h"
# include "bittorque-icon-64.h"
#endif

#include "bittorque-default-config.h"

static gchar *bittorque_private_dir = NULL;
static guint  bittorque_local_port = 6881;

static const GOptionEntry bittorque_option_entries[] = {
	{"private-directory", 0, 0,
	 G_OPTION_ARG_FILENAME,
	 &bittorque_private_dir,
	 N_("Use this directory for configuration and other data, useful if running from e.g. a USB stick"),
	 NULL},

	{"local-port", 0, 0,
	 G_OPTION_ARG_INT,
	 &bittorque_local_port,
	 N_("Use this as the default port for incoming connections"),
	 NULL},

	{NULL, 0, 0, 0, NULL, NULL, NULL}
};

BittorqueApp bittorque;

static void
bittorque_load_widgets (GtkBuilder *builder)
{
//	GtkTreeViewColumn *col;
//	GtkCellRenderer   *cell;
	GtkTreeSelection  *selection;

	bittorque.main_window = GTK_WIDGET (gtk_builder_get_object (builder, "bittorque_window"));

	bittorque.preferences_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "preferences_dialog"));

	bittorque.torrents_treeview = GTK_WIDGET (gtk_builder_get_object (builder, "torrents_treeview"));
	bittorque.torrents_list = GTK_LIST_STORE (gtk_builder_get_object (builder, "torrents_model"));//gtk_list_store_new (1, BT_TYPE_TORRENT);
/*
	bittorque.torrents_list = gtk_list_store_new (1, BT_TYPE_TORRENT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (bittorque.torrents_treeview), GTK_TREE_MODEL (bittorque.torrents_list));
	g_object_unref (bittorque.torrents_list);

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Status"));

	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_cell_data_func (col, cell, torrents_status_cell_renderer_func, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (bittorque.torrents_treeview), col);

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Name"));
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_min_width (col, 200);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_cell_data_func (col, cell, torrents_name_cell_renderer_func, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (bittorque.torrents_treeview), col);

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Size"));
	gtk_tree_view_column_set_resizable (col, TRUE);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cell, TRUE);
	gtk_tree_view_column_set_cell_data_func (col, cell, torrents_size_cell_renderer_func, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (bittorque.torrents_treeview), col);
*/
	gtk_tree_view_column_set_cell_data_func (GTK_TREE_VIEW_COLUMN(gtk_builder_get_object (builder, "treeviewcolumn1")), GTK_CELL_RENDERER(gtk_builder_get_object (builder, "treeviewcolumn1-renderer1")), torrents_status_cell_renderer_func, NULL, NULL);
	gtk_tree_view_column_set_cell_data_func (GTK_TREE_VIEW_COLUMN(gtk_builder_get_object (builder, "treeviewcolumn2")), GTK_CELL_RENDERER(gtk_builder_get_object (builder, "treeviewcolumn2-renderer1")), torrents_name_cell_renderer_func, NULL, NULL);
	gtk_tree_view_column_set_cell_data_func (GTK_TREE_VIEW_COLUMN(gtk_builder_get_object (builder, "treeviewcolumn3")), GTK_CELL_RENDERER(gtk_builder_get_object (builder, "treeviewcolumn3-renderer1")), torrents_size_cell_renderer_func, NULL, NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (bittorque.torrents_treeview));

	gtk_tree_selection_set_select_function (selection, torrents_view_selection_func, NULL, NULL);
}

static void
bittorque_log_handler (const gchar *domain, GLogLevelFlags flags, const gchar *message, gpointer data)
{
	g_log_default_handler (domain, flags, message, data);
}

static GKeyFile *
bittorque_load_default_config ()
{
	GKeyFile *config = g_key_file_new ();

	g_key_file_load_from_data (config, bittorque_default_config, bittorque_default_config_len, G_KEY_FILE_NONE, NULL);

	return config;
}

static GKeyFile *
bittorque_open_config_file (gchar **filename)
{
	GKeyFile *config = g_key_file_new ();
	GError *error = NULL;
	gchar *config_file = NULL;
	gboolean loaded = FALSE;

	if (bittorque_private_dir != NULL) {
		config_file = g_build_filename (bittorque_private_dir, "bittorque.cfg", NULL);

		if (!g_key_file_load_from_file (config, config_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)) {
			if (error->domain == G_KEY_FILE_ERROR)
				g_message (_("Error loading configuration from private directory, skipping: %s"), error->message);

			g_clear_error (&error);

			g_free (config_file);
		} else {
			if (filename != NULL)
				*filename = config_file;

			loaded = TRUE;
		}
	}

	if (!loaded) {
		config_file = g_build_filename (g_get_user_config_dir (), "bittorque", "bittorque.cfg", NULL);

		if (!g_key_file_load_from_file (config, config_file, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)) {
			if (error->domain == G_KEY_FILE_ERROR)
				g_message (_("Error loading configuration from user directory: %s"), error->message);

			g_clear_error (&error);

			g_free (config_file);
		} else {
			if (filename != NULL)
				*filename = config_file;

			loaded = TRUE;
		}
	}

	if (!loaded) {
		g_message (_("Using default configuration..."));

		if (filename != NULL) {
			if (bittorque_private_dir != NULL)
				*filename = g_build_filename (bittorque_private_dir, "bittorque.cfg", NULL);
			else
				*filename = g_build_filename (g_get_user_config_dir (), "bittorque", "bittorque.cfg", NULL);
		}
	}

	return config;
}

static GtkBuilder *
bittorque_open_ui ()
{
	GtkBuilder *builder = gtk_builder_new ();

#ifndef BITTORQUE_EMBED_DATA
	gchar    *ui_file = NULL;
	gboolean  found = FALSE;

	/* look for the UI file, first in private directory if specified, then BITTORQUE_DATA_DIR, then in current directory */
	if (bittorque_private_dir != NULL) {
		ui_file = g_build_filename (bittorque_private_dir, "bittorque.ui", NULL);
		if (g_file_test (ui_file, G_FILE_TEST_EXISTS)) {
			gtk_builder_add_from_file (builder, ui_file, NULL);
			found = TRUE;
		}
		g_free (ui_file);
	}

	if (!found) {
		if (g_file_test (BITTORQUE_DATA_DIR "bittorque.ui", G_FILE_TEST_EXISTS)) {
			gtk_builder_add_from_file (builder, BITTORQUE_DATA_DIR "bittorque.ui", NULL);
			found = TRUE;
		}
	}

	if (!found) {
		g_printerr (_("Couldn't find bittorque.ui - check your installation!\n"));
		return NULL;
	}
#else
	gtk_builder_add_from_string (builder, bittorque_ui, sizeof (bittorque_ui) / sizeof (bittorque_ui[0]), NULL);
#endif

	return builder;
}

GdkPixbuf *
bittorque_icon_from_size (gint size, GError **error)
{
	g_return_val_if_fail (size > 0, NULL);

#ifdef BITTORQUE_EMBED_DATA
	if (size <= 16)
		return gdk_pixbuf_new_from_inline (-1, bittorque_icon_16, FALSE, error);
	else if (size <= 24)
		return gdk_pixbuf_new_from_inline (-1, bittorque_icon_24, FALSE, error);
	else
		return gdk_pixbuf_new_from_inline (-1, bittorque_icon_64, FALSE, error);
#else
	if (bittorque_private_dir != NULL) {
		g_warning ("NOT IMPLEMENTED: Loading icon from private directory");
	}

	if (size <= 16)
		return gdk_pixbuf_new_from_file (BITTORQUE_DATA_DIR "icons" G_DIR_SEPARATOR_S "bittorque16.png", error);
	else if (size <= 24)
		return gdk_pixbuf_new_from_file (BITTORQUE_DATA_DIR "icons" G_DIR_SEPARATOR_S "bittorque24.png", error);
	else
		return gdk_pixbuf_new_from_file (BITTORQUE_DATA_DIR "icons" G_DIR_SEPARATOR_S "bittorque64.png", error);
#endif

	return NULL;
}

int
main (int argc, char *argv[])
{
	GtkBuilder     *builder = NULL;
	GError         *error = NULL;
	GOptionContext *context = NULL;

#ifdef ENABLE_NLS
	gchar          *locale_dir = NULL;
#endif

#ifdef BITTORQUE_EMBED_DATA
	GdkPixdata      pixdata G_GNUC_UNUSED;
#endif

	/* apparently this has to be first */
	if (!g_thread_supported ())
		g_thread_init (NULL);

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, BITTORQUE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	/* register our own log handler for all logs */
	g_log_set_handler (NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_RECURSION | G_LOG_FLAG_FATAL, bittorque_log_handler, NULL);

	/* initialize gnet */
	gnet_init ();

	/* argument parsing */
	context = g_option_context_new (_("- lightweight BitTorrent client for GTK"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_set_help_enabled (context, TRUE);
	g_option_context_set_description (context, _("BitTorque is a lightweight BitTorrent client for GTK. See www.bittorque.org to learn more."));
	g_option_context_add_main_entries (context, bittorque_option_entries, NULL);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	g_option_context_parse (context, &argc, &argv, &error);

	if (error != NULL) {
		g_printerr (_("%s\nRun with \'--help\' to see a list of available command-line options.\n"), error->message);
		return 1;
	}

	g_option_context_free (context);

#ifdef ENABLE_NLS
	if (bittorque_private_dir != NULL) {
		locale_dir = g_build_path (bittorque_private_dir, "locale", NULL);
		bindtextdomain (GETTEXT_PACKAGE, locale_dir);
		g_free (locale_dir);
	}
#endif

	/* set the locale */
	gtk_set_locale ();

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), BITTORQUE_DATA_DIR "icons" G_DIR_SEPARATOR_S);

	bittorque.config = bittorque_open_config_file (&(bittorque.config_file));

	bittorque.default_config = bittorque_load_default_config ();

	builder = bittorque_open_ui ();

	g_return_val_if_fail (builder != NULL, 1);

	bittorque_load_widgets (builder);

	gtk_builder_connect_signals (builder, NULL);

//	g_object_unref (builder);

	bittorque.manager = g_object_new (BT_TYPE_MANAGER, "port", bittorque_local_port, NULL);

	if (!bt_manager_start_accepting (bittorque.manager, &error)) {
		g_warning ("could not start listening on port");
	}

	bittorque.status_icon = gtk_status_icon_new ();

	gtk_status_icon_set_from_icon_name (bittorque.status_icon, "bittorque"); /*bittorque_icon_from_size (gtk_status_icon_get_size (bittorque.status_icon), NULL));*/

	gtk_status_icon_set_visible (bittorque.status_icon, TRUE);

	gtk_status_icon_set_tooltip (bittorque.status_icon, "BitTorque");

	gtk_widget_show_all (bittorque.main_window);

	gtk_main ();

	g_object_unref (bittorque.manager);

	return 0;
}
