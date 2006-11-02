#include <glib.h>

#include "rc4.h"


#define swap(x, y) { guchar __swap = (x); (x) = (y); (y) = __swap; }


void
rc4_prepare (RC4Context *rc4, const gchar *key, gsize len)
{
	gint i;

	guchar x, y;
	guchar *state;

	rc4->x = rc4->y = x = y = 0;
	state = rc4->state;

	for (i = 0; i < 256; i++)
		state[i] = i;

	for (i = 0; i < 256; i++) {
		y = (key[x] + state[i] + y) % 256;
		swap (state[i], state[y]);
		x = (x + 1) % len;
	}
}

 
void
rc4_cipher (RC4Context *rc4, gchar *buf, gsize len)
{ 
	gsize i;

	guchar x, y, xor;
	guchar *state;

	x = rc4->x;
	y = rc4->y;
	state = rc4->state;

	for (i = 0; i < len; i++) {
		x = (x + 1) % 256;
		y = (state[x] + y) % 256;
		swap (state[x], state[y]);
		xor = state[x] + state[y] % 256;
		buf[i] ^= state[xor];
	}

	rc4->x = x;
	rc4->y = y;
}
