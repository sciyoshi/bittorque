#ifndef __TORQUE_TORRENT_PEER__
#define __TORQUE_TORRENT_PEER__

#include <glib.h>
#include <glib-object.h>

#include "torque-utils.h"
#include "torque-torrent.h"

#define TORQUE_TYPE_TORRENT_PEER (torque_torrent_peer_get_type ())
#define TORQUE_TORRENT_PEER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TORQUE_TYPE_TORRENT_PEER, TorqueTorrentPeer))

typedef struct _TorqueTorrentPeer TorqueTorrentPeer;
typedef struct _TorqueTorrentPeerClass TorqueTorrentPeerClass;

GType torque_torrent_peer_get_type ();

TorqueTorrentPeer *torque_torrent_peer_new (TorqueTorrent *torrent);

#endif
