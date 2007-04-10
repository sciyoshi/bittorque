/**
 * bt-peer-extension.c
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

#include "bt-peer-extension.h"

GType
bt_peer_extension_get_type ()
{
	static GType type = 0;

	if (!type) {
		const GTypeInfo info = {
			sizeof (BtPeerExtensionIface),
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL,
			NULL
		};
		
		type = g_type_register_static (G_TYPE_INTERFACE, "BtPeerExtension", &info, 0);
	}

	return type;
}

