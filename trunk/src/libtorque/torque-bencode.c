#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "torque-bencode.h"
#include "torque-utils.h"

static GString *_bencode (TorqueBEncodedData *data, GString **string);

#define TORQUE_BENCODE_ERROR_TEXT "syntax error in file near character %d: %s"

void
torque_bencode_destroy (TorqueBEncodedData *data)
{
	g_return_if_fail (data != NULL);

	switch (data->type) {
	case TORQUE_BENCODED_STRING:
		g_string_free (data->string, TRUE);
		break;

	case TORQUE_BENCODED_LIST:
		g_slist_foreach (data->list, (GFunc) torque_bencode_destroy, NULL);
		g_slist_free (data->list);
		break;

	case TORQUE_BENCODED_DICT:
		g_tree_destroy (data->dict);
		break;

	case TORQUE_BENCODED_INT:
		break;
	}

	g_slice_free (TorqueBEncodedData, data);
}


static TorqueBEncodedData *
_bdecode (gchar *start, gchar *buf, gchar **end, GError **error)
{
	TorqueBEncodedData *data = NULL;
	gchar *tmp = NULL;

	if (end == NULL)
		end = &tmp;

	if (*buf == 'i') {

		/* Integers are encoded like this: 'i12345e' */
		gint64 i = strtoll (buf + 1, end, 10);

		if (*end == buf + 1 || **end != 'e') {
			g_set_error (error, TORQUE_ERROR, TORQUE_ERROR_INVALID_BENCODED_DATA, TORQUE_BENCODE_ERROR_TEXT, *end - start, "invalid integer");
			return NULL;
		}

		(*end)++;

		data = g_slice_new0 (TorqueBEncodedData);
		data->type = TORQUE_BENCODED_INT;
		data->value = i;
		return data;

	} else if (g_ascii_isdigit (*buf)) {

		/* Strings are encoded like this: '15:abcdefghijklmno' */
		gint len = strtol (buf, end, 10);

		if (**end != ':') {
			g_set_error (error, TORQUE_ERROR, TORQUE_ERROR_INVALID_BENCODED_DATA, TORQUE_BENCODE_ERROR_TEXT, *end - start, "invalid string");
			return NULL;
		}

		data = g_slice_new0 (TorqueBEncodedData);
		data->type = TORQUE_BENCODED_STRING;
		data->string = g_string_new_len (*end + 1, len);
		*end += len + 1;
		return data;

	} else if (*buf == 'd') {

		/* Dictionaries are like so: 'd(bencodedkey)(bencodedval)(key)(val)...e' */
		GTree *dict = g_tree_new_full ((GCompareDataFunc) strcmp, NULL, g_free, (GDestroyNotify) torque_bencode_destroy);

		*end = buf + 1;

		while (**end != 'e') {
			TorqueBEncodedData *key, *val;

			key = _bdecode (start, *end, end, error);

			if (key == NULL) {
				g_tree_destroy (dict);
				return NULL;
			}

			if (key->type != TORQUE_BENCODED_STRING) {
				g_set_error (error, TORQUE_ERROR, TORQUE_ERROR_INVALID_BENCODED_DATA, TORQUE_BENCODE_ERROR_TEXT, *end - start, "key must be string");
				g_tree_destroy (dict);
				return NULL;
			}

			val = _bdecode (start, *end, end, error);

			if (val == NULL) {
				torque_bencode_destroy (key);
				g_tree_destroy (dict);
				return NULL;
			}

			g_tree_insert (dict, g_strdup (key->string->str), val);
			torque_bencode_destroy (key);
		}

		(*end)++;

		data = g_slice_new0 (TorqueBEncodedData);
		data->type = TORQUE_BENCODED_DICT;
		data->dict = dict;
		return data;

	} else if (*buf == 'l') {

		/* ...and lists are like this: 'l(bencodeditem)(item)(item)...e' */
		GSList *list = NULL;

		*end = buf + 1;

		while (**end != 'e') {
			TorqueBEncodedData *val = _bdecode (start, *end, end, error);

			if (val == NULL) {
				g_slist_foreach (list, (GFunc) torque_bencode_destroy, NULL);
				return NULL;
			}

			list = g_slist_prepend (list, val);
		}

		list = g_slist_reverse (list);

		(*end)++;

		data = g_slice_new0 (TorqueBEncodedData);
		data->type = TORQUE_BENCODED_LIST;
		data->list = list;
		return data;
	}

	g_set_error (error, TORQUE_ERROR, TORQUE_ERROR_INVALID_BENCODED_DATA, TORQUE_BENCODE_ERROR_TEXT, *end - start, "unknown element or EOF");
	return NULL;
}


TorqueBEncodedData *
torque_bencode_decode (gchar *buf, GError **error)
{
	return _bdecode (buf, buf, NULL, error);
}

static gboolean
_bencode_key_val (gchar *key, TorqueBEncodedData *val, GString **string)
{
	g_string_append_printf (*string, "%d:%s", strlen (key), key);
	*string = _bencode (val, string);
	return FALSE;
}

static GString *
_bencode (TorqueBEncodedData *data, GString **string)
{
	GString *str;

	if (string == NULL) {
		string = &str;
		*string = g_string_sized_new (100);
	}

	switch (data->type) {
	case TORQUE_BENCODED_INT:
		g_string_append_printf (*string, "i%llde", data->value);
		break;

	case TORQUE_BENCODED_STRING:
		g_string_append_printf (*string, "%d:", data->string->len);
		*string = g_string_append_len (*string, data->string->str, data->string->len);
		break;

	case TORQUE_BENCODED_DICT:
		*string = g_string_append_c (*string, 'd');
		g_tree_foreach(data->dict, (GTraverseFunc) _bencode_key_val, (gpointer) string);
		*string = g_string_append_c (*string, 'e');
		break;

	case TORQUE_BENCODED_LIST:
		*string = g_string_append_c (*string, 'l');
		g_slist_foreach(data->list, (GFunc) _bencode, (gpointer) string);
		*string = g_string_append_c (*string, 'e');
		break;
	}

	return *string;
}


GString *
torque_bencode_encode (TorqueBEncodedData *data)
{
	return _bencode (data, NULL);
}
