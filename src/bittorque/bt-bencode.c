#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "bt-bencode.h"
#include "bt-utils.h"

#define BT_BENCODE_ERROR_TEXT "syntax error in bencoded dict: %s"

static GString *_bt_bencode_encode (BtBencode *data, GString **string);

static BtBencode *
_bt_bencode_decode (const gchar *buf, const gchar **end, GError **error)
{
	BtBencode   *data = NULL;
	const gchar *tmp  = NULL;

	if (end == NULL)
		end = &tmp;

	if (*buf == 'i') {

		/* Integers are encoded like this: 'i12345e' */
		gint64 i = strtoll (buf + 1, (char **) end, 10);

		if (*end == buf + 1 || **end != 'e') {
			g_set_error (error, BT_ERROR, BT_ERROR_INVALID_BENCODED_DATA, BT_BENCODE_ERROR_TEXT, "invalid integer");
			return NULL;
		}

		(*end)++;

		data = g_slice_new0 (BtBencode);
		data->type = BT_BENCODE_TYPE_INT;
		data->value.value = i;
		return data;

	} else if (g_ascii_isdigit (*buf)) {

		/* Strings are encoded like this: '15:abcdefghijklmno' */
		gint len = strtol (buf, (char **) end, 10);

		if (**end != ':') {
			g_set_error (error, BT_ERROR, BT_ERROR_INVALID_BENCODED_DATA, BT_BENCODE_ERROR_TEXT, "invalid string");
			return NULL;
		}

		data = g_slice_new0 (BtBencode);
		data->type = BT_BENCODE_TYPE_STRING;
		data->value.string = g_string_new_len (*end + 1, len);
		*end += len + 1;
		return data;

	} else if (*buf == 'd') {

		/* Dictionaries are like so: 'd(bencodedkey)(bencodedval)(key)(val)...e' */
		GTree *dict = g_tree_new_full ((GCompareDataFunc) strcmp, NULL, g_free, (GDestroyNotify) bt_bencode_destroy);

		*end = buf + 1;

		while (**end != 'e') {
			BtBencode *key, *val;

			key = _bt_bencode_decode (*end, end, error);

			if (key == NULL) {
				g_tree_destroy (dict);
				return NULL;
			}

			if (key->type != BT_BENCODE_TYPE_STRING) {
				g_set_error (error, BT_ERROR, BT_ERROR_INVALID_BENCODED_DATA, BT_BENCODE_ERROR_TEXT, "key must be string");
				g_tree_destroy (dict);
				return NULL;
			}

			val = _bt_bencode_decode (*end, end, error);

			if (val == NULL) {
				bt_bencode_destroy (key);
				g_tree_destroy (dict);
				return NULL;
			}

			g_tree_insert (dict, g_strdup (key->value.string->str), val);
			bt_bencode_destroy (key);
		}

		(*end)++;

		data = g_slice_new0 (BtBencode);
		data->type = BT_BENCODE_TYPE_DICT;
		data->value.dict = dict;
		return data;

	} else if (*buf == 'l') {

		/* ...and lists are like this: 'l(bencodeditem)(item)(item)...e' */
		GSList *list = NULL;

		*end = buf + 1;

		while (**end != 'e') {
			BtBencode *val = _bt_bencode_decode (*end, end, error);

			if (val == NULL) {
				g_slist_foreach (list, (GFunc) bt_bencode_destroy, NULL);
				return NULL;
			}

			list = g_slist_prepend (list, val);
		}

		list = g_slist_reverse (list);

		(*end)++;

		data = g_slice_new0 (BtBencode);
		data->type = BT_BENCODE_TYPE_LIST;
		data->value.list = list;
		return data;
	}

	g_set_error (error, BT_ERROR, BT_ERROR_INVALID_BENCODED_DATA, BT_BENCODE_ERROR_TEXT, "unknown element or unexpected EOF");
	return NULL;
}

static gboolean
_bt_bencode_encode_key_val (gchar *key, BtBencode *val, GString **string)
{
	g_string_append_printf (*string, "%d:%s", strlen (key), key);
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
		g_string_append_printf (*string, "i%llde", data->value.value);
		break;

	case BT_BENCODE_TYPE_STRING:
		g_string_append_printf (*string, "%d:", data->value.string->len);
		*string = g_string_append_len (*string, data->value.string->str, data->value.string->len);
		break;

	case BT_BENCODE_TYPE_DICT:
		*string = g_string_append_c (*string, 'd');
		g_tree_foreach (data->value.dict, (GTraverseFunc) _bt_bencode_encode_key_val, (gpointer) string);
		*string = g_string_append_c (*string, 'e');
		break;

	case BT_BENCODE_TYPE_LIST:
		*string = g_string_append_c (*string, 'l');
		g_slist_foreach (data->value.list, (GFunc) _bt_bencode_encode, (gpointer) string);
		*string = g_string_append_c (*string, 'e');
		break;
	}

	return *string;
}

/**
 * bt_bencode_destroy:
 *
 * Frees all resources associated with the bencoded data.
 */

void
bt_bencode_destroy (BtBencode *data)
{
	g_return_if_fail (data != NULL);

	switch (data->type) {
	case BT_BENCODE_TYPE_STRING:
		g_string_free (data->value.string, TRUE);
		break;

	case BT_BENCODE_TYPE_LIST:
		g_slist_foreach (data->value.list, (GFunc) bt_bencode_destroy, NULL);
		g_slist_free (data->value.list);
		break;

	case BT_BENCODE_TYPE_DICT:
		g_tree_destroy (data->value.dict);
		break;

	case BT_BENCODE_TYPE_INT:
		break;
	}

	g_slice_free (BtBencode, data);
}

/**
 * bt_bencode_decode:
 *
 * Decodes the given string into a BtBencode structure.
 */

BtBencode *
bt_bencode_decode (const gchar *buf, GError **error)
{
	return _bt_bencode_decode (buf, NULL, error);
}


/**
 * bt_bencode_encode:
 *
 * Encodes the given BtBencode structure into a GString
 */

GString *
bt_bencode_encode (BtBencode *data)
{
	return _bt_bencode_encode (data, NULL);
}
