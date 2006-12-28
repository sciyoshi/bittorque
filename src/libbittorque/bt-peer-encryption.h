/**
 * bt-peer-encryption.h
 *
 * Copyright 2006 Samuel Cormier-Iijima <sciyoshi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St,, Fifth Floor, Boston, MA 02110-1301, USA
 */

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
