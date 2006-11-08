#include "bt-peer.h"
#include "bt-peer-encryption.h"

gboolean
bt_peer_encryption_init (BtPeer *peer)
{
	g_return_val_if_fail (BT_IS_PEER (peer), FALSE);

	return TRUE;
}
