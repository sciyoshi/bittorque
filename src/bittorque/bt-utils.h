#ifndef __BT_UTILS_H__
#define __BT_UTILS_H__

#include <glib.h>

#include <gnet.h>

#include "bt-bencode.h"
#include "bt-manager.h"

#define BT_ERROR (bt_error_get_quark ())

typedef enum {
	BT_ERROR_NETWORK,
	BT_ERROR_RESOLVING,
	BT_ERROR_INVALID_BENCODED_DATA,
	BT_ERROR_INVALID_TORRENT,
	BT_ERROR_PEER_HANDSHAKE,
	BT_ERROR_TRACKER,
	BT_ERROR_INVALID
} BtError;

GQuark   bt_error_get_quark ();

gboolean bt_infohash (BtBencode *info, gchar hash[20]);

gboolean bt_create_peer_id (gchar id[21]);

gchar   *bt_url_encode (const gchar *string, gsize size);

gboolean bt_hash_to_string (gchar hash[20], gchar string[41]);

GSource *bt_io_source_create (BtManager *manager, GIOChannel *channel, GIOCondition condition, GSourceFunc callback, gpointer data);

GSource *bt_timeout_source_create (BtManager *manager, guint timeout, GSourceFunc callback, gpointer data);

GSource *bt_idle_source_create (BtManager *manager, GSourceFunc callback, gpointer data);

#endif
