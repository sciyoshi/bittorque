#ifndef __BT_BENCODE_H__
#define __BT_BENCODE_H__

#include <glib.h>

#define bt_bencode_lookup(t, v) ((BtBencode *) g_tree_lookup (((BtBencode *) t)->value.dict, v))
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
	} value;
} BtBencode;

void       bt_bencode_destroy (BtBencode *data);

BtBencode *bt_bencode_decode (const gchar *buf, GError **error);

GString   *bt_bencode_encode (BtBencode *data);

G_END_DECLS

#endif
