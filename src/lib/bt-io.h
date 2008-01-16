/**
 * bt-io.h
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

#ifndef __BT_IO_H__
#define __BT_IO_H__

#include <glib-object.h>

#define BT_TYPE_IO    (bt_io_get_type ())
#define BT_IO(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_IO, BtIO))
#define BT_IS_IO(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_IO))

typedef struct _BtIO      BtIO;
typedef struct _BtIOClass BtIOClass;

#include "bt-torrent.h"

GType            bt_io_get_type ();

void             bt_io_write (BtIO *io, guint piece, guint begin, guint len, const gchar* data);

gchar           *bt_io_read (BtIO *io, guint piece, guint begin, guint len);

gboolean         bt_io_check_piece_hash (BtIO *io, guint piece);

#endif
