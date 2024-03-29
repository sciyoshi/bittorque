/**
 * bt-torrent-file.h
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

#ifndef __BT_TORRENT_FILE__
#define __BT_TORRENT_FILE__

#include <glib-object.h>

typedef struct {
	/* full name (including path) of the file */
	gchar *name;
	
	/* size of the file in bytes */
	gsize  size;
	
	/* offset of this file within the torrent */
	gsize  offset;
	
	/* priority of this file - 0 means don't download, 1-9 are rankings */
	gint   priority;
} BtTorrentFile;

GType bt_torrent_file_get_type ();

#endif
