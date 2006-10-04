#ifndef __TORQUE_UTILS_H__
#define __TORQUE_UTILS_H__

#include <glib.h>

#include "torque-bencode.h"

#define TORQUE_ERROR (torque_error_quark ())

typedef enum {
	TORQUE_ERROR_INVALID_TORRENT,
	TORQUE_ERROR_INVALID_BENCODED_DATA,
} TorqueError;

GQuark torque_error_quark ();
gboolean torque_create_peer_id(gchar id[21]);
gboolean torque_info_hash (TorqueBEncodedData *info, gchar hash[20]);
gboolean torque_hash_to_string (gchar hash[20], gchar string[40]);
GSList *torque_strv_to_slist (gchar **str_array);

#endif
