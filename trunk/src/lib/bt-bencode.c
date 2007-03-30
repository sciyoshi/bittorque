/**
 * bt-bencode.c
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

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "bt-bencode.h"

/* error text for encoding problems */
#define BT_BENCODE_ERROR_TEXT "syntax error in bencoded dict: %s"

static GString *_bt_bencode_encode (BtBencode *data, GString **string);

static BtBencode *
_bt_bencode_decode (const gchar *buf, const gchar **end, const gchar *max, GError **error)
{
	BtBencode   *data = NULL;
	const gchar *tmp  = NULL;

	if (end == NULL)
		end = &tmp;

	if (*buf == 'i') {

		/* integers are encoded like this: 'i12345e' */
		gint64 i = g_ascii_strtoll (buf + 1, (char **) end, 10);

		if (*end == buf + 1 || **end != 'e' || *end >= max) {
			g_set_error (error, BT_BENCODE_ERROR, BT_BENCODE_ERROR_INVALID, BT_BENCODE_ERROR_TEXT, "invalid integer");
			return NULL;
		}

		(*end)++;

		data = g_slice_new0 (BtBencode);
		data->type = BT_BENCODE_TYPE_INT;
		data->value = i;
		return data;

	} else if (g_ascii_isdigit (*buf)) {

		/* strings are encoded like this: '15:abcdefghijklmno' */
		gint len = strtol (buf, (gchar **) end, 10);

		if (*end + len + 1 >= max) {
			g_set_error (error, BT_BENCODE_ERROR, BT_BENCODE_ERROR_INVALID, BT_BENCODE_ERROR_TEXT, "unexpected EOF");
			return NULL;
		}

		if (**end != ':') {
			g_set_error (error, BT_BENCODE_ERROR, BT_BENCODE_ERROR_INVALID, BT_BENCODE_ERROR_TEXT, "invalid string");
			return NULL;
		}

		data = g_slice_new0 (BtBencode);
		data->type = BT_BENCODE_TYPE_STRING;
		data->string = g_string_new_len (*end + 1, len);
		*end += len + 1;
		return data;

	} else if (*buf == 'd') {

		/* dictionaries are like so: 'd(bencodedkey)(bencodedval)(key)(val)...e' */
		GTree *dict = g_tree_new_full ((GCompareDataFunc) strcmp, NULL, g_free, (GDestroyNotify) bt_bencode_destroy);

		*end = buf + 1;

		while (**end != 'e') {
			BtBencode *key, *val;

			key = _bt_bencode_decode (*end, end, max, error);

			if (key == NULL) {
				g_assert (error == NULL || *error != NULL);
				g_tree_destroy (dict);
				return NULL;
			}

			if (key->type != BT_BENCODE_TYPE_STRING) {
				g_set_error (error, BT_BENCODE_ERROR, BT_BENCODE_ERROR_INVALID, BT_BENCODE_ERROR_TEXT, "key must be string");
				g_tree_destroy (dict);
				return NULL;
			}

			val = _bt_bencode_decode (*end, end, max, error);

			if (val == NULL) {
				g_assert (error == NULL || *error != NULL);
				bt_bencode_destroy (key);
				g_tree_destroy (dict);
				return NULL;
			}

			g_tree_insert (dict, g_strdup (key->string->str), val);
			bt_bencode_destroy (key);
		}

		(*end)++;

		data = g_slice_new0 (BtBencode);
		data->type = BT_BENCODE_TYPE_DICT;
		data->dict = dict;
		return data;

	} else if (*buf == 'l') {

		/* ...and lists are like this: 'l(bencodeditem)(item)(item)...e' */
		GSList *list = NULL;

		*end = buf + 1;

		while (**end != 'e') {
			BtBencode *val = _bt_bencode_decode (*end, end, max, error);

			if (val == NULL) {
				g_assert (error == NULL || *error != NULL);
				g_slist_foreach (list, (GFunc) bt_bencode_destroy, NULL);
				return NULL;
			}

			list = g_slist_prepend (list, val);
		}

		list = g_slist_reverse (list);

		(*end)++;

		data = g_slice_new0 (BtBencode);
		data->type = BT_BENCODE_TYPE_LIST;
		data->list = list;
		return data;
	}

	g_set_error (error, BT_BENCODE_ERROR, BT_BENCODE_ERROR_INVALID, BT_BENCODE_ERROR_TEXT, "unknown element or unexpected EOF");
	return NULL;
}

static gboolean
_bt_bencode_encode_key_val (gchar *key, BtBencode *val, GString **string)
{
	g_string_append_printf (*string, "%Zd:%s", strlen (key), key);
	*string = _bt_bencode_encode (val, string);
	return FALSE;
}

static GString *
_bt_bencode_encode (BtBencode *data, GString **string)
{
	GString *str;

	if (string == NULL) {
		string = &str;
		*string = g_string_sized_new (100);
	}

	switch (data->type) {
	case BT_BENCODE_TYPE_INT:
		g_string_append_printf (*string, "i%" G_GINT64_MODIFIER "de", data->value);
		break;

	case BT_BENCODE_TYPE_STRING:
		g_string_append_printf (*string, "%Zd:", data->string->len);
		*string = g_string_append_len (*string, data->string->str, data->string->len);
		break;

	case BT_BENCODE_TYPE_DICT:
		*string = g_string_append_c (*string, 'd');
		g_tree_foreach (data->dict, (GTraverseFunc) _bt_bencode_encode_key_val, (gpointer) string);
		*string = g_string_append_c (*string, 'e');
		break;

	case BT_BENCODE_TYPE_LIST:
		*string = g_string_append_c (*string, 'l');
		g_slist_foreach (data->list, (GFunc) _bt_bencode_encode, (gpointer) string);
		*string = g_string_append_c (*string, 'e');
		break;
	}

	return *string;
}

/**
 * bt_bencode_error_quark:
 *
 * Gets the error quark for BEncoding errors. It is better to use #BT_BENCODE_ERROR instead.
 *
 * Returns: the quark
 */
GQuark
bt_bencode_error_quark ()
{
	return g_quark_from_static_string ("BtBencodeError");
}

/**
 * bt_bencode_destroy:
 * @data: the data to free
 *
 * Frees all resources associated with the bencoded data.
 */
void
bt_bencode_destroy (BtBencode *data)
{
	g_return_if_fail (data != NULL);

	switch (data->type) {
	case BT_BENCODE_TYPE_STRING:
		g_string_free (data->string, TRUE);
		break;

	case BT_BENCODE_TYPE_LIST:
		g_slist_foreach (data->list, (GFunc) bt_bencode_destroy, NULL);
		g_slist_free (data->list);
		break;

	case BT_BENCODE_TYPE_DICT:
		g_tree_destroy (data->dict);
		break;

	case BT_BENCODE_TYPE_INT:
		break;
	}

	g_slice_free (BtBencode, data);
}

/**
 * bt_bencode_decode:
 * @buf: the buffer to decode
 * @len: the length of the buffer
 * @error: a return location for errors
 *
 * Decodes the given string into a #BtBencode structure.
 *
 * Returns: the decoded BEncode structure
 */
BtBencode *
bt_bencode_decode (const gchar *buf, gsize len, GError **error)
{
	BtBencode *bencode;
	const gchar *end = NULL;

	bencode = _bt_bencode_decode (buf, &end, buf + len, error);

	if (!bencode)
		return NULL;

	if (end != buf + len) {
		g_set_error (error, BT_BENCODE_ERROR, BT_BENCODE_ERROR_INVALID, BT_BENCODE_ERROR_TEXT, "leftover or not enough characters");
		return NULL;
	}

	return bencode;
}

/**
 * bt_bencode_encode:
 * @data: the encoded structure
 *
 * Encodes the given #BtBencode structure into a GString
 *
 * Returns: a #GString with the BEncoded data
 */
GString *
bt_bencode_encode (BtBencode *data)
{
	return _bt_bencode_encode (data, NULL);
}
