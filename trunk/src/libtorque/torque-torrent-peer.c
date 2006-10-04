#include "torque-torrent-peer.h"
#include "torque-torrent.h"

typedef enum {
	PROPERTY_IP = 1,
	PROPERTY_TORRENT,
} TorqueTorrentProperty;

struct _TorqueTorrentPeer {
	GObject parent;

	guint64 ip;
	TorqueTorrent *torrent;

	gboolean choking;     /// we are choking the peer
	gboolean interesting; /// we are interested in the peer
	gboolean choked;      /// the peer is choking us
	gboolean interested;  /// the peer is interested in us

	gboolean disposed;
};

struct _TorqueTorrentPeerClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (TorqueTorrentPeer, torque_torrent_peer, G_TYPE_OBJECT)

static void
torque_torrent_peer_dispose ()
{

}

static void
torque_torrent_peer_finalize ()
{

}

static void
torque_torrent_peer_set_property (GObject *obj, guint property, const GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	TorqueTorrentPeer *self = TORQUE_TORRENT_PEER (obj);

	switch (property) {
	case PROPERTY_IP:
		self->ip = g_value_get_float (value);
		break;

	case PROPERTY_TORRENT:
		self->torrent = TORQUE_TORRENT (g_value_dup_object (value));
		break;
	}
}

static void
torque_torrent_peer_get_property (GObject *obj, guint property, GValue *value, GParamSpec *pspec G_GNUC_UNUSED)
{
	TorqueTorrentPeer *self = TORQUE_TORRENT_PEER (obj);

	switch (property) {
	case PROPERTY_IP:
		g_value_set_uint64 (value, self->ip);
		break;

	case PROPERTY_TORRENT:
		g_value_set_object (value, self->torrent);
		break;
	}
}

static void
torque_torrent_peer_init (TorqueTorrentPeer *self)
{
	self->ip = 0;
	self->disposed = FALSE;

	self->choking = TRUE;
	self->choked = TRUE;
	self->interesting = FALSE;
	self->interested = FALSE;

	return;
}

static void
torque_torrent_peer_class_init (TorqueTorrentPeerClass *klass)
{
	GParamSpec *param;

	GObjectClass *g_class = G_OBJECT_CLASS (klass);

	g_class->get_property = torque_torrent_peer_get_property;
	g_class->set_property = torque_torrent_peer_set_property;
	g_class->dispose = torque_torrent_peer_dispose;
	g_class->finalize = torque_torrent_peer_finalize;

	param = g_param_spec_uint64 ("ip", "peer ip address", "This peer's IP address.", 0, G_MAXUINT64, 0, G_PARAM_READWRITE);
	g_object_class_install_property (g_class, PROPERTY_IP, param);

	param = g_param_spec_object ("torrent", "torrent for this peer", "The torrent that this peer is serving.", TORQUE_TYPE_TORRENT, G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
	g_object_class_install_property (g_class, PROPERTY_TORRENT, param);

	return;
}

TorqueTorrentPeer *
torque_torrent_peer_new (TorqueTorrent *torrent)
{
	TorqueTorrentPeer *peer = TORQUE_TORRENT_PEER (g_object_new (TORQUE_TYPE_TORRENT_PEER, NULL));

	peer->torrent = g_object_ref (torrent);

	return peer;
}
