/**
 * bt-bencode.h
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

#ifndef __BT_BENCODE_H__
#define __BT_BENCODE_H__

#include <glib.h>

#define bt_bencode_lookup(t, v) ((BtBencode *) g_tree_lookup (((BtBencode *) t)->dict, v))
#define bt_bencode_slitem(x)    ((BtBencode *) ((x)->data))

G_BEGIN_DECLS

typedef enum {
	BT_BENCODE_TYPE_INT,
	BT_BENCODE_TYPE_STRING,
	BT_BENCODE_TYPE_LIST,
	BT_BENCODE_TYPE_DICT
} BtBencodeType;

typedef struct {
	BtBencodeType type;
	union {
		gint64 value;
		GString *string;
		GSList *list;
		GTree *dict;
	};
} BtBencode;

void       bt_bencode_destroy (BtBencode *data);

BtBencode *bt_bencode_decode (const gchar *buf, GError **error);

GString   *bt_bencode_encode (BtBencode *data);

G_END_DECLS

#endif
