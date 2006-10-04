#ifndef __TORQUE_TORRENT_H__
#define __TORQUE_TORRENT_H__

#include <glib.h>
#include <glib-object.h>

#include "torque-utils.h"

#define TORQUE_TYPE_TORRENT (torque_torrent_get_type ())
#define TORQUE_TORRENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TORQUE_TYPE_TORRENT, TorqueTorrent))

typedef struct _TorqueTorrent TorqueTorrent;
typedef struct _TorqueTorrentClass TorqueTorrentClass;

GType torque_torrent_get_type ();

TorqueTorrent *torque_torrent_new (const gchar *uri, GError **error);

const gchar *torque_torrent_get_info_hash (TorqueTorrent *torrent);
const gchar *torque_torrent_get_name (TorqueTorrent *torrent);

#endif
