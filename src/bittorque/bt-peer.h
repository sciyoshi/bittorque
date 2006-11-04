#ifndef __BT_PEER_H__
#define __BT_PEER_H__

#include <glib.h>
#include <glib-object.h>

#include <gnet.h>

G_BEGIN_DECLS

#define BT_TYPE_PEER    (bt_peer_get_type ())
#define BT_PEER(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BT_TYPE_PEER, BtPeer))
#define BT_IS_PEER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BT_TYPE_PEER))

#define BT_TYPE_PEER_STATUS      (bt_peer_status_get_type ())

typedef struct _BtPeer      BtPeer;
typedef struct _BtPeerClass BtPeerClass;

#include "bt-manager.h"
#include "bt-torrent.h"

typedef enum {
	BT_PEER_STATUS_CONNECTING,
	BT_PEER_STATUS_CONNECTED_SEND,
	BT_PEER_STATUS_CONNECTED_WAIT,
	BT_PEER_STATUS_SEND_HANDSHAKE,
	BT_PEER_STATUS_WAIT_HANDSHAKE,
	BT_PEER_STATUS_HANDSHAKE_PEER_ID,
	BT_PEER_STATUS_DISCONNECTING,
	BT_PEER_STATUS_DISCONNECTED,
	BT_PEER_STATUS_IDLE_HAVE,
	BT_PEER_STATUS_IDLE
} BtPeerStatus;

struct _BtPeer {
	GObject       parent;

	BtManager    *manager;
	BtTorrent    *torrent;
	BtPeerStatus  status;
	GConn        *socket;
	GInetAddr    *address;

	GString      *buffer;
	gsize         pos;

	gchar        *bitmask;

	gboolean      choking;
	gboolean      interesting;
	gboolean      choked;
	gboolean      interested;
};

struct _BtPeerClass {
	GObjectClass parent;
};

GType    bt_peer_get_type ();
GType    bt_peer_status_get_type ();

void     bt_peer_set_choking (BtPeer *self, gboolean choked);

void     bt_peer_set_interesting (BtPeer *self, gboolean choked);

gboolean bt_peer_get_choking (BtPeer *self);

gboolean bt_peer_get_interesting (BtPeer *self);

gboolean bt_peer_get_choked (BtPeer *self);

gboolean bt_peer_get_interested (BtPeer *self);

BtPeer  *bt_peer_new (BtManager *manager, BtTorrent *torrent, GTcpSocket *socket, GInetAddr *address);

void     bt_peer_receive (BtPeer *peer, gchar *buffer, gsize length);

G_END_DECLS

#endif
