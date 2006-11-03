#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>

#include "bt-utils.h"
#include "bt-bencode.h"
#include "sha1.h"

static const char hex_alphabet[] = "0123456789ABCDEF";

GQuark
bt_error_get_quark ()
{
	return g_quark_from_static_string ("bittorque");
}

gboolean
bt_infohash (BtBencode *info, gchar hash[20])
{
	SHA1Context sha;
	GString    *string;

	g_return_val_if_fail (info != NULL && hash != NULL, FALSE);

	string = bt_bencode_encode (info);

	sha1_init (&sha);
	sha1_update (&sha, string->str, string->len);
	sha1_finish (&sha, hash);

	g_string_free (string, TRUE);
	return TRUE;
}

gboolean
bt_create_peer_id (gchar id[21])
{
	SHA1Context sha;
	pid_t       pid;
	GTimeVal    t;
	gchar       tmp[20];
	guint       i;

	g_return_val_if_fail (id != NULL, FALSE);

	pid = getpid ();

	g_snprintf (id, 21, "-TO%02d%02d-", 0, 1);
	sha1_init (&sha);
	sha1_update (&sha, (gchar *) &pid, sizeof (pid_t));

	g_get_current_time(&t);
	sha1_update (&sha, (gchar *) &t, sizeof (GTimeVal));
	sha1_finish (&sha, tmp);

	for (i = 8; i < 20; i++)
		id[i] = hex_alphabet[(tmp[i] & 0xF)];

	id[20] = '\0';
	return TRUE;
}

gchar *
bt_url_encode (const gchar *string, gsize size)
{
	gchar *ret;
	gsize  len, i;

	len = 0;
	ret = g_malloc0 (size * 3 + 1);

	for (i = 0; i < size; i++) {
		if (!g_ascii_isalnum (string[i])) {
			ret[len++] = '%';
			ret[len++] = hex_alphabet[(string[i] & 0xF0) >> 4];
			ret[len++] = hex_alphabet[string[i] & 0xF];
		} else {
			ret[len++] = string[i];
		}
	}

	ret = g_realloc (ret, len + 1);
	ret[len] = '\0';
	return ret;
}

gboolean
bt_hash_to_string (gchar hash[20], gchar string[41])
{
	guint i;

	g_return_val_if_fail (hash != NULL && string != NULL, FALSE);

	for (i = 0; i < 20; i++) {
		string[2 * i] = hex_alphabet[(hash[i] >> 4) & 0xF];
		string[2 * i + 1] = hex_alphabet[hash[i] & 0xF];
	}

	string[40] = '\0';
	return TRUE;
}

GSource *
bt_io_source_create (BtManager *manager, GIOChannel *channel, GIOCondition condition, GSourceFunc callback, gpointer data)
{
	GSource *source;
	g_return_val_if_fail (BT_IS_MANAGER (manager), NULL);
	source = g_io_create_watch (channel, condition);
	g_source_set_callback (source, callback, data, NULL);
	bt_manager_add_source (manager, source);
	return source;
}

GSource *
bt_timeout_source_create (BtManager *manager, guint timeout, GSourceFunc callback, gpointer data)
{
	GSource *source;
	source = g_timeout_source_new (timeout);
	g_source_set_callback (source, (GSourceFunc) callback, data, NULL);
	bt_manager_add_source (manager, source);
	return source;
}

GSource *
bt_idle_source_create (BtManager *manager, GSourceFunc callback, gpointer data)
{
	GSource *source;
	source = g_idle_source_new ();
	g_source_set_callback (source, (GSourceFunc) callback, data, NULL);
	bt_manager_add_source (manager, source);
	return source;
}
