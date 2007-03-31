/**
 * bittorque-ui.h
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

#ifndef __BITTORQUE_UI__
#define __BITTORQUE_UI__

#include <gtk/gtk.h>

void     torrents_status_cell_renderer_func (GtkTreeViewColumn *col,
                                             GtkCellRenderer   *renderer,
                                             GtkTreeModel      *model,
                                             GtkTreeIter       *iter,
                                             gpointer           data);

void     torrents_size_cell_renderer_func   (GtkTreeViewColumn *col,
                                             GtkCellRenderer   *renderer,
                                             GtkTreeModel      *model,
                                             GtkTreeIter       *iter,
                                             gpointer           data);

void     torrents_name_cell_renderer_func   (GtkTreeViewColumn *col,
                                             GtkCellRenderer   *renderer,
                                             GtkTreeModel      *model,
                                             GtkTreeIter       *iter,
                                             gpointer           data);

gboolean torrents_view_selection_func (GtkTreeSelection *selection,
                                       GtkTreeModel     *model,
                                       GtkTreePath      *path,
                                       gboolean          currently_selected,
                                       gpointer          data);


void bittorque_open_torrent ();

void bittorque_open_preferences ();

#endif
