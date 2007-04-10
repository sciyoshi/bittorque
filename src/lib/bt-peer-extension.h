/**
 * bt-peer-extension.h
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

#ifndef __BT_PEER_EXTENSION_H__
#define __BT_PEER_EXTENSION_H__

#include <glib-object.h>

#define BT_TYPE_PEER_EXTENSION            (bt_peer_extension_get_type ())
#define BT_PEER_EXTENSION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PEER_EXTENSION, BtPeerExtension))
#define BT_IS_PEER_EXTENSION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PEER_EXTENSION))
#define BT_PEER_EXTENSION_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), BT_TYPE_PEER_EXTENSION, BtPeerExtensionIface))

typedef struct _BtPeerExtension      BtPeerExtension;
typedef struct _BtPeerExtensionIface BtPeerExtensionIface;

struct _BtPeerExtensionIface {
	GTypeInterface parent;
};

GType bt_peer_extension_get_type ();

#endif
