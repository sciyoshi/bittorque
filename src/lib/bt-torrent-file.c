/**
 * bt-torrent-file.c
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

#include "bt-torrent-file.h"

static gpointer
bt_torrent_file_copy (gpointer torrent_file)
{
	BtTorrentFile *file, *new;
	
	file = (BtTorrentFile *) torrent_file;
	
	new = g_slice_new0 (BtTorrentFile);
	
	new->name = g_strdup (file->name);
	
	new->size = file->size;
	new->offset = file->offset;
	new->priority = file->priority;
	
	return new;
}

static void
bt_torrent_file_delete (gpointer torrent_file)
{
	BtTorrentFile *file = (BtTorrentFile *) torrent_file;

	g_free (file->name);

	g_slice_free (BtTorrentFile, file);
}

GType
bt_torrent_file_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		type = g_boxed_type_register_static ("BtTorrentFile", bt_torrent_file_copy, bt_torrent_file_delete);
	}

	return type;
}
