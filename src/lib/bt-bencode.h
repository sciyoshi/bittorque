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

/**
 * BT_BENCODE_ERROR:
 *
 * Error domain for BEncoding errors.
 */
#define BT_BENCODE_ERROR (bt_bencode_error_quark ())

/**
 * bt_bencode_lookup:
 * @bencode: the #BtBencode *dictionary* to look for an item in
 * @name: the key to look up
 *
 * Convenience macro to find the item with the given key in a BEncoded dictionary.
 *
 * Returns: the value associated with the key, as another #BtBencode pointer
 */
#define bt_bencode_lookup(bencode, name) ((BtBencode *) g_tree_lookup (((BtBencode *) bencode)->dict, name))

/**
 * bt_bencode_slitem:
 * @list: a #GSList pointer
 *
 * Convenience macro for getting items in #BtBencode lists. For example:
 *
 * <informalexample><programlisting>
 * BtBEncode *bencoded;
 * GSList *i;
 * for (i = bencoded->list; i != NULL; i = i->next) {
 * 	BtBencode *item = bt_bencode_slitem (i);
 * 	do something with item...
 * }
 * </programlisting></informalexample>
 *
 * Returns: the list element as a #BtBencode pointer
 */
#define bt_bencode_slitem(list)    ((BtBencode *) ((list)->data))

G_BEGIN_DECLS

/**
 * BtBencodeType:
 * @BT_BENCODE_TYPE_INT: indicates that the BEncoded integer can be accessed through value
 * @BT_BENCODE_TYPE_STRING: indicates that the BEncoded string can be accessed as a #GString through the string element
 * @BT_BENCODE_TYPE_LIST: indicates that the BEncoded list can be accessed through the list element
 * @BT_BENCODE_TYPE_DICT: indicates that the BEncoded dictionary can be accessed through the dict element
 *
 * The different types of BEncoded data. The actual data can be accessed through the union in the structure.
 */
typedef enum {
	BT_BENCODE_TYPE_INT,
	BT_BENCODE_TYPE_STRING,
	BT_BENCODE_TYPE_LIST,
	BT_BENCODE_TYPE_DICT
} BtBencodeType;

/**
 * BtBencodeError:
 * @BT_BENCODE_ERROR_INVALID: generic encoding error
 *
 * Various error types for BEncoding and decoding operations
 */
typedef enum {
	BT_BENCODE_ERROR_INVALID
} BtBencodeError;

/**
 * BtBencode:
 * @type: a #BtBencodeType specifying the type of data
 * @value: a #gint64 that holds the integer value, if type is %BT_BENCODE_TYPE_INT
 * @string: a #GString that holds the string, if type is %BT_BENCODE_TYPE_STRING
 * @list: a #GSList that holds a singly linked list of other #BtBencode structures, if type is %BT_BENCODE_TYPE_LIST
 * @dict: a #GTree that holds string key -> #BtBencode mappings, if type is %BT_BENCODE_TYPE_DICT
 *
 * Represents BEncoded data.
 */
typedef struct {
	BtBencodeType type;
	union {
		gint64 value;
		GString *string;
		GSList *list;
		GTree *dict;
	};
} BtBencode;

GQuark     bt_bencode_error_quark ();

void       bt_bencode_destroy (BtBencode *data);

BtBencode *bt_bencode_decode (const gchar *buf, gsize len, GError **error);

GString   *bt_bencode_encode (BtBencode *data);

G_END_DECLS

#endif
