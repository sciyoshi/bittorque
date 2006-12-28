#include <stdio.h>
#include <stdlib.h>

#include "bt-peer.h"
#include "bt-peer-encryption.h"

#define PRIME_P "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA" \
                "63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C2" \
                "45E485B576625E7EC6F44C42E9A63A36210000000000090563"

#ifdef HAVE_GCRYPT_H

static gcry_mpi_t prime_p = NULL;

static gcry_mpi_t generator_g = NULL;

#endif

void
bt_peer_encryption_receive (BtPeer *self, gchar *buf G_GNUC_UNUSED, gsize len G_GNUC_UNUSED, BtPeerEncryption *encryption)
{
	g_return_if_fail (BT_IS_PEER (self));
	g_return_if_fail (encryption != NULL);

	if (self->status == BT_PEER_STATUS_CONNECTED_WAIT) {

	}
}

void
bt_peer_encryption_send_diffie_hellman (BtPeer *self, BtPeerEncryption *encryption)
{
	gchar x_a[20];
	gchar y_a[96];
	gchar *pad;
	gsize pad_size;

	g_return_if_fail (BT_IS_PEER (self));
	g_return_if_fail (encryption != NULL);

#ifdef HAVE_GCRYPT_H
	encryption->y_a = gcry_mpi_new (768);
	gcry_randomize (x_a, 20, GCRY_STRONG_RANDOM);
	gcry_mpi_scan (&(encryption->x_a), GCRYMPI_FMT_USG, x_a, 20, NULL);
	gcry_mpi_powm (encryption->y_a, generator_g, encryption->x_a, prime_p);
	gcry_mpi_print (GCRYMPI_FMT_USG, (guchar *) y_a, 96, NULL, encryption->y_a);
#endif

	pad_size = g_random_int_range (0, 512);
	pad = g_malloc (pad_size);

#ifdef HAVE_GCRYPT_H
	gcry_randomize (pad, pad_size, GCRY_STRONG_RANDOM);
#endif

	gnet_conn_write (self->socket, y_a, 96);
	gnet_conn_write (self->socket, pad, pad_size);
}

gboolean
bt_peer_encryption_init (BtPeer *self, BtPeerEncryptionMode mode G_GNUC_UNUSED)
{
	BtPeerEncryption *encryption;

	g_return_val_if_fail (BT_IS_PEER (self), FALSE);

	encryption = g_new0 (BtPeerEncryption, 1);

#ifdef HAVE_GCRYPT_H
	if (G_UNLIKELY (prime_p == NULL)) {
		gcry_mpi_scan (&prime_p, GCRYMPI_FMT_HEX, PRIME_P, 0, NULL);
		gcry_mpi_scan (&generator_g, GCRYMPI_FMT_HEX, "2", 0, NULL);
	}
#endif

	if (self->status == BT_PEER_STATUS_CONNECTED_SEND) {
		g_signal_connect (self, "data-read", G_CALLBACK (bt_peer_encryption_receive), encryption);
		bt_peer_encryption_send_diffie_hellman (self, encryption);
		encryption->status = BT_PEER_ENCRYPTION_STATUS_WAIT_DH_B;
	} else if (self->status == BT_PEER_STATUS_CONNECTED_WAIT) {
		g_signal_connect_after (self, "data-read", G_CALLBACK (bt_peer_encryption_receive), encryption);
	}

	return TRUE;
}
