#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "torque-utils.h"
#include "torque-bencode.h"

#include "sha1.h"

static const char hex_alphabet[] = "0123456789ABCDEF";

GQuark
torque_error_quark ()
{
	return g_quark_from_static_string ("torque error");
}

gboolean
torque_create_peer_id(gchar id[21])
{
	SHA1Context sha;
	pid_t pid;
	GTimeVal t;
	gchar tmp[20];

	g_return_val_if_fail (id != NULL, FALSE);

	pid = getpid ();

	sprintf (id, "-TO%02d%02d-", TORQUE_VERSION_MAJOR, TORQUE_VERSION_MINOR);
	sha1_init (&sha);
	sha1_update (&sha, (gchar *) &pid, sizeof (pid_t));

	g_get_current_time(&t);
	sha1_update (&sha, (gchar *) &t, sizeof (GTimeVal));
	sha1_finish (&sha, tmp);

	for (guint i = 8; i < 20; i++)
		id[i] = (tmp[i] & 0xF) > 9 ? ((tmp[i] & 0xF) - 0xA + 'A') : ((tmp[i] & 0xF) + '0');

	id[20] = '\0';

	return TRUE;
}

gboolean
torque_info_hash (TorqueBEncodedData *info, gchar hash[20])
{
	g_return_val_if_fail (info != NULL && hash != NULL, FALSE);

	SHA1Context sha;
	GString *string = torque_bencode_encode (info);

	sha1_init (&sha);
	sha1_update (&sha, string->str, string->len);
	sha1_finish (&sha, hash);

	g_string_free (string, TRUE);

	return TRUE;
}


gboolean
torque_hash_to_string (gchar hash[20], gchar string[41])
{
	g_return_val_if_fail (hash != NULL && string != NULL, FALSE);

	for (guint i = 0; i < 20; i++) {
		string[2 * i] = hex_alphabet[(hash[i] >> 4) & 0xF];
		string[2 * i + 1] = hex_alphabet[hash[i] & 0xF];
	}

	string[40] = '\0';

	return TRUE;
}

GSList *
torque_strv_to_slist (gchar **str_array)
{
	GSList *ret;
	guint len;

	g_return_val_if_fail (str_array, NULL);

	ret = NULL;
	len = g_strv_length (str_array);

	for (guint pos = 0; pos < len; pos++)
		ret = g_slist_prepend (ret, g_strdup (str_array[pos]));

	return g_slist_reverse (ret);

}
