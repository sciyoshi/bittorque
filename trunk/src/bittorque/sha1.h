#ifndef __SHA1_H__
#define __SHA1_H__

#include <glib.h>

#define SHA1_DATA_SIZE 64
#define SHA1_DIGEST_SIZE 20

typedef struct {
	guint32 digest[5];
	guint32 data[16];
	guint32 low, high;
} SHA1Context;

void sha1_init (SHA1Context *sha);
void sha1_update (SHA1Context *sha, const gchar *buf, gsize len);
void sha1_finish (SHA1Context *sha, gchar *out);

#endif
