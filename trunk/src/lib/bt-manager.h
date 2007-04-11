/**
 * bt-manager.h
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

#ifndef __BT_MANAGER_H__
#define __BT_MANAGER_H__

#include <glib-object.h>

#define BT_TYPE_MANAGER    (bt_manager_get_type ())
#define BT_MANAGER(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MANAGER, BtManager))
#define BT_IS_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MANAGER))

typedef struct _BtManager      BtManager;
typedef struct _BtManagerClass BtManagerClass;

#include "bt-torrent.h"

GType            bt_manager_get_type ();

BtManager       *bt_manager_new ();

void             bt_manager_add_torrent (BtManager *manager, BtTorrent *torrent);

void             bt_manager_remove_torrent (BtManager *manager);

BtTorrent       *bt_manager_get_torrent (BtManager *manager, gchar *id);

void             bt_manager_stop_accepting (BtManager *manager);

gboolean         bt_manager_start_accepting (BtManager *manager, GError **error);

gushort          bt_manager_get_port (BtManager *manager);

void             bt_manager_set_port (BtManager *manager, gushort port);

const gchar     *bt_manager_get_peer_id (BtManager *manager);

void             bt_manager_set_peer_id (BtManager *manager, const gchar *peer_id);

#endif
