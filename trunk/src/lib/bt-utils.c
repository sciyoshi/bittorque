/**
 * bt-utils.c
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

#include <sys/types.h>
#include <errno.h>
#include <string.h>

#include "bt-utils.h"
#include "bt-bencode.h"
#include "bt-manager.h"
#include "sha1.h"

static const char bt_hex_alphabet[] = "0123456789ABCDEF";

GQuark
bt_error_quark ()
{
	return g_quark_from_static_string ("BitTorque"); 
}

static gpointer
bt_tcp_socket_copy (gpointer socket)
{
	if (socket == NULL)
		return NULL;

	gnet_tcp_socket_ref ((GTcpSocket *) socket);
	
	return socket;
}

static void
bt_tcp_socket_unref (gpointer socket)
{
	if (socket == NULL)
		return;
	
	gnet_tcp_socket_unref ((GTcpSocket *) socket);
}

GType
bt_tcp_socket_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		type = g_boxed_type_register_static ("BtTcpSocket", bt_tcp_socket_copy, bt_tcp_socket_unref);
	}

	return type;
}

static gpointer
bt_inet_addr_copy (gpointer addr)
{
	if (addr == NULL)
		return NULL;
	
	gnet_inetaddr_ref ((GInetAddr *) addr);
	
	return addr;
}

static void
bt_inet_addr_unref (gpointer addr)
{
	if (addr == NULL)
		return;

	gnet_inetaddr_unref ((GInetAddr *) addr);
}

GType
bt_inet_addr_get_type ()
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		type = g_boxed_type_register_static ("BtInetAddr", bt_inet_addr_copy, bt_inet_addr_unref);
	}

	return type;
}

gchar *
bt_get_infohash (BtBencode *info)
{
	SHA1Context  sha;
	gchar       *hash;
	GString     *string;

	g_return_val_if_fail (info != NULL, NULL);

	hash = g_malloc (20);

	string = bt_bencode_encode (info);

	sha1_init (&sha);
	sha1_update (&sha, string->str, string->len);
	sha1_finish (&sha, hash);

	g_string_free (string, TRUE);

	return hash;
}

gchar *
bt_create_peer_id ()
{
	SHA1Context sha;
	GTimeVal t;
	gchar tmp[20];
	gchar *id;
	guint i;

	id = g_malloc (21);

	g_snprintf (id, 21, "-TO%02d%02d-", 0, 1);

	sha1_init (&sha);

	g_get_current_time (&t);
	sha1_update (&sha, (gchar *) &t, sizeof (GTimeVal));
	sha1_finish (&sha, tmp);

	for (i = 8; i < 20; i++)
		id[i] = bt_hex_alphabet[(tmp[i] & 0xF)];

	id[20] = '\0';

	return id;
}

gchar *
bt_url_encode (const gchar *string, gsize size)
{
	gchar *ret;
	gsize len, i;

	g_return_val_if_fail (string != NULL, NULL);

	len = 0;
	ret = g_malloc0 (size * 3 + 1);

	for (i = 0; i < size; i++) {
		if (!g_ascii_isalnum (string[i])) {
			ret[len++] = '%';
			ret[len++] = bt_hex_alphabet[(string[i] & 0xF0) >> 4];
			ret[len++] = bt_hex_alphabet[string[i] & 0xF];
		} else {
			ret[len++] = string[i];
		}
	}

	ret = g_realloc (ret, len + 1);
	ret[len] = '\0';
	return ret;
}

gchar *
bt_hash_to_string (const gchar *hash)
{
	guint i;
	gchar *string;

	g_return_val_if_fail (hash != NULL, NULL);

	string = g_malloc (41);

	for (i = 0; i < 20; i++) {
		string[2 * i] = bt_hex_alphabet[(hash[i] >> 4) & 0xF];
		string[2 * i + 1] = bt_hex_alphabet[hash[i] & 0xF];
	}

	string[40] = '\0';

	return string;
}

/**
 * bt_client_name_from_id:
 * @id: the peer id
 *
 * Finds the name of a peer's client, given his peer id.
 *
 * Returns: a newly allocated string with the name of the client, to be freed with g_free
 */
gchar *
bt_client_name_from_id (const gchar *id)
{
	g_return_val_if_fail (id != NULL, NULL);

	if (id[0] == '-' && id[7] == '-') {
		if (!memcmp (&id[1], "TR", 2))
			return g_strdup_printf ("Transmission %d.%d", (id[3] - '0') * 10 + (id[4] - '0'), (id[5] - '0') * 10 + (id[6] - '0'));

		if (!memcmp (&id[1], "AZ", 2))
			return g_strdup_printf ("Azureus %c.%c.%c.%c", id[3], id[4], id[5], id[6]);

		if (!memcmp (&id[1], "UT", 2))
			return g_strdup_printf ("μTorrent %c.%c.%c.%c", id[3], id[4], id[5], id[6]);

		if (!memcmp (&id[1], "TS", 2))
			return g_strdup_printf ("TorrentStorm (%c%c%c%c)", id[3], id[4], id[5], id[6]);

		if (!memcmp (&id[1], "BC", 2))
			return g_strdup_printf ("BitComet %d.%c%c", (id[3] - '0') * 10 + (id[4] - '0'), id[5], id[6]);

		if (!memcmp (&id[1], "SZ", 2))
			return g_strdup_printf ("Shareaza %c.%c.%c.%c", id[3], id[4], id[5], id[6]);
	}

	if (!memcmp (&id[4], "----", 4)) {
		if(id[0] == 'T')
			return g_strdup_printf ("BitTornado (%c%c%c)", id[1], id[2], id[3]);

		else if(id[0] == 'A')
			return g_strdup_printf ("ABC (%c%c%c)", id[1], id[2], id[3]);
	}

	if (id[0] == 'M' && id[2] == '-' && id[4] == '-' && id[6] == '-' && id[7] == '-')
		return g_strdup_printf ("BitTorrent %c.%c.%c", id[1], id[3], id[5]);

	if (!memcmp (id, "exbc", 4))
		return g_strdup_printf ("BitComet %d.%02d", id[4], id[5]);


	return g_strdup_printf ("Unknown client (%c%c%c%c%c%c%c%c)", id[0], id[1], id[2], id[3], id[4], id[5], id[6], id[7]);
}

/**
 * bt_size_to_string:
 * @size: the size to convert to a string
 *
 * Gives a textual representation of a size in bytes.
 *
 * Returns: the newly allocated string, to be freed with g_free
 */
gchar *
bt_size_to_string (guint64 size)
{
	if (size < 1024)
		return g_strdup_printf ("%d B", (guint32) size);
	if (size < 1048576)
		return g_strdup_printf ("%d kB", (guint32) (size / 1024));
	if (size < 1073741824)
		return g_strdup_printf ("%.2f MB", ((gdouble) (size / 1024)) / 1024);

	return g_strdup_printf ("%.2f GB", ((gdouble) (size / (1048576))) / 1024);
}

/* workaround for -fstrict-aliasing */
void
bt_add_weak_pointer (GObject* obj, gpointer pointer_to_weak_pointer)
{
	if (obj == NULL)
		return;

	g_object_add_weak_pointer (obj, (gpointer*)pointer_to_weak_pointer);
}

/* workaround for -fstrict-aliasing */
void
bt_remove_weak_pointer (GObject* obj, gpointer pointer_to_weak_pointer)
{
	if (obj == NULL)
		return;

	g_object_remove_weak_pointer (obj, (gpointer*)pointer_to_weak_pointer);
}

