/**
 * bittorque-callbacks.c
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
#include "bittorque-ui.h"

BT_EXPORT void
on_preferences_menuitem_activate (GtkMenuItem *menuitem G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	bittorque_open_preferences ();
}

BT_EXPORT void
on_plugins_menuitem_activate (GtkMenuItem *menuitem G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{

}

BT_EXPORT void
on_open_torrent_menuitem_activate (GtkMenuItem *menuitem G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	bittorque_open_torrent ();
}

BT_EXPORT void
on_create_torrent_menuitem_activate (GtkMenuItem *menuitem G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{

}

BT_EXPORT void
on_quit_menuitem_activate (GtkMenuItem *menuitem G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	gtk_main_quit ();
}

BT_EXPORT void
bittorque_about_callback (GtkMenuItem *menuitem G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{

}

BT_EXPORT void
bittorque_open_torrent_callback (GtkToolButton *toolbutton G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
	bittorque_open_torrent ();
}

BT_EXPORT void
on_create_torrent_toolbutton_clicked (GtkToolButton *toolbutton G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{

}
