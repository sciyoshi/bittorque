#ifndef __TORQUE_FILE_H__
#define __TORQUE_FILE_H__

#include <glib.h>
#include <glib-object.h>

#include "torque-torrent.h"

#define TORQUE_TYPE_TORRENT_FILE (torque_torrent_file_get_type ())
#define TORQUE_TORRENT_FILE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TORQUE_TYPE_TORRENT_FILE, TorqueTorrentFile))

typedef struct _TorqueTorrentFile TorqueTorrentFile;
typedef struct _TorqueTorrentFileClass TorqueTorrentFileClass;

GType torque_torrent_file_get_type ();

TorqueTorrentFile *torque_torrent_file_new (TorqueTorrent *torrent, gchar *name, gsize size);

#endif
