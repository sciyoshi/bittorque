#ifndef __BENCODE_H__
#define __BENCODE_H__

#include <glib.h>

#define torque_bencode_lookup(t, v) ((TorqueBEncodedData *) g_tree_lookup (((TorqueBEncodedData *) t)->dict, v))
#define torque_bencode_slitem(x) ((TorqueBEncodedData *) ((x)->data))

typedef enum {
	TORQUE_BENCODED_INT,
	TORQUE_BENCODED_STRING,
	TORQUE_BENCODED_LIST,
	TORQUE_BENCODED_DICT,
} TorqueBEncodedType;

typedef struct {
	TorqueBEncodedType type;
	gint64 value;
	GString *string;
	GSList *list;
	GTree *dict;
} TorqueBEncodedData;

void torque_bencode_destroy(TorqueBEncodedData *data);
TorqueBEncodedData *torque_bencode_decode(gchar *buf, GError **error);
GString *torque_bencode_encode(TorqueBEncodedData *data);

#endif
