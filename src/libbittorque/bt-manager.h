/**
 * bt-manager.h
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

#ifndef __BT_MANAGER_H__
#define __BT_MANAGER_H__

#include <glib.h>
#include <glib-object.h>
#include <gnet.h>

G_BEGIN_DECLS

#define BT_TYPE_MANAGER         (bt_manager_get_type ())
#define BT_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_MANAGER, BtManager))
#define BT_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BT_TYPE_MANAGER, BtManagerClass))
#define BT_IS_MANAGER(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_MANAGER))

typedef struct _BtManager      BtManager;
typedef struct _BtManagerClass BtManagerClass;

#include "bt-torrent.h"

struct _BtManager {
	GObject       parent;

	/* the main context this manager should run in */
	GMainContext *context;

	/* the GTcpSocket that we are listening for connections on */
	GTcpSocket   *accept_socket;

	/* a hash table holding a infohash->torrent mapping */
	GHashTable   *torrents;

	/* the peer id that torrents should use */
	gchar        *peer_id;

	/* the port we are listening on */
	gushort       port;

	/* the private directory we should use to store private info in */
	gchar        *private_dir;

	GKeyFile     *preferences;
};

struct _BtManagerClass {
	GObjectClass parent;
};

GType      bt_manager_get_type ();

BtManager *bt_manager_new ();

BtManager *bt_manager_new_with_main (GMainContext *context);

guint      bt_manager_add_source (BtManager *manager, GSource *source);

gboolean   bt_manager_accept_start (BtManager *self, GError **error);

void       bt_manager_accept_stop (BtManager *self);

gboolean   bt_manager_set_port (BtManager *self, gushort port, GError **error);

gushort    bt_manager_get_port (BtManager *self);

void       bt_manager_add_torrent (BtManager *self, BtTorrent *torrent);

BtTorrent *bt_manager_get_torrent (BtManager *self, const gchar *infohash);

BtTorrent *bt_manager_get_torrent_string (BtManager *self, const gchar *infohash);

G_END_DECLS

#endif
