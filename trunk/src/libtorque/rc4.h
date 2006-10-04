#ifndef __RC4_H__
#define __RC4_H__

#include <glib.h>

typedef struct {
	guchar state[256];
	guchar x, y;
} RC4Context;

void rc4_prepare (RC4Context *rc4, const gchar *key, gsize len);
void rc4_cipher (RC4Context *rc4, gchar *buf, gsize len);

#endif
