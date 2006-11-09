#ifndef __BT_PEER_ENCRYPTION_H__
#define __BT_PEER_ENCRYPTION_H__

#ifdef HAVE_GMP_H
# include <gmp.h>
#endif

#ifdef HAVE_GCRYPT_H
# include <gcrypt.h>
#endif

#include "bt-peer.h"

typedef enum {
	BT_PEER_ENCRYPTION_STATUS_WAIT_DH_A,
	BT_PEER_ENCRYPTION_STATUS_WAIT_DH_B,
	BT_PEER_ENCRYPTION_STATUS_IDLE
} BtPeerEncryptionStatus;

typedef enum {
	BT_PEER_ENCRYPTION_MODE_NONE = 1,
	BT_PEER_ENCRYPTION_MODE_FALLBACK,
	BT_PEER_ENCRYPTION_MODE_ONLY
} BtPeerEncryptionMode;

typedef struct {
	BtPeerEncryptionStatus status;
#ifdef HAVE_GCRYPT_H
	gcry_mpi_t x_a;
	gcry_mpi_t y_a;
	gcry_mpi_t x_b;
	gcry_mpi_t y_b;
	gcry_mpi_t s;
#endif

} BtPeerEncryption;

gboolean bt_peer_encryption_init (BtPeer *peer, BtPeerEncryptionMode encryption);

#endif
