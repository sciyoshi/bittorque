#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "sha1.h"


#define f1(x, y, z) (z ^ (x & (y ^ z)))
#define f2(x, y, z) (x ^ y ^ z)
#define f3(x, y, z) ((x & y) | (z & (x | y)))
#define f4(x, y, z) (x ^ y ^ z)

#define k1 0x5A827999L
#define k2 0x6ED9EBA1L
#define k3 0x8F1BBCDCL
#define k4 0xCA62C1D6L

#define h0 0x67452301L
#define h1 0xEFCDAB89L
#define h2 0x98BADCFEL
#define h3 0x10325476L
#define h4 0xC3D2E1F0L

#define lrotate(n, x)  (((x) << n) | ((x) >> (32 - n)))
#define expand(w, i) (w[i & 15] = lrotate (1, (w[i & 15] ^ w[(i - 14) & 15] ^ w[(i - 8) & 15] ^ w[(i - 3) & 15])))
#define round(a, b, c, d, e, f, k, w) (e += lrotate (5, a) + f(b, c, d) + k + w, b = lrotate (30, b))


static void
sha1_to_be (guint32 *data, guint count)
{
	for (guint i = 0; i < count / sizeof (guint32); i++)
		data[i] = GUINT32_TO_BE (data[i]);
}


static void
sha1_from_be (guint32 *data, guint count)
{
	for (guint i = 0; i < count / sizeof (guint32); i++)
		data[i] = GUINT32_FROM_BE (data[i]);
}


void
sha1_init (SHA1Context *sha)
{
	sha->digest[0] = h0;
	sha->digest[1] = h1;
	sha->digest[2] = h2;
	sha->digest[3] = h3;
	sha->digest[4] = h4;
	sha->low = sha->high = 0;
}


static void
sha1_digest (guint32 digest[5], guint32 data[16])
{
	guint32 a, b, c, d, e;
	guint32 w[16];

	a = digest[0];
	b = digest[1];
	c = digest[2];
	d = digest[3];
	e = digest[4];

	memcpy (w, data, SHA1_DATA_SIZE);

	round (a, b, c, d, e, f1, k1, w[0]);
	round (e, a, b, c, d, f1, k1, w[1]);
	round (d, e, a, b, c, f1, k1, w[2]);
	round (c, d, e, a, b, f1, k1, w[3]);
	round (b, c, d, e, a, f1, k1, w[4]);
	round (a, b, c, d, e, f1, k1, w[5]);
	round (e, a, b, c, d, f1, k1, w[6]);
	round (d, e, a, b, c, f1, k1, w[7]);
	round (c, d, e, a, b, f1, k1, w[8]);
	round (b, c, d, e, a, f1, k1, w[9]);
	round (a, b, c, d, e, f1, k1, w[10]);
	round (e, a, b, c, d, f1, k1, w[11]);
	round (d, e, a, b, c, f1, k1, w[12]);
	round (c, d, e, a, b, f1, k1, w[13]);
	round (b, c, d, e, a, f1, k1, w[14]);
	round (a, b, c, d, e, f1, k1, w[15]);
	round (e, a, b, c, d, f1, k1, expand(w, 16));
	round (d, e, a, b, c, f1, k1, expand(w, 17));
	round (c, d, e, a, b, f1, k1, expand(w, 18));
	round (b, c, d, e, a, f1, k1, expand(w, 19));

	round (a, b, c, d, e, f2, k2, expand(w, 20));
	round (e, a, b, c, d, f2, k2, expand(w, 21));
	round (d, e, a, b, c, f2, k2, expand(w, 22));
	round (c, d, e, a, b, f2, k2, expand(w, 23));
	round (b, c, d, e, a, f2, k2, expand(w, 24));
	round (a, b, c, d, e, f2, k2, expand(w, 25));
	round (e, a, b, c, d, f2, k2, expand(w, 26));
	round (d, e, a, b, c, f2, k2, expand(w, 27));
	round (c, d, e, a, b, f2, k2, expand(w, 28));
	round (b, c, d, e, a, f2, k2, expand(w, 29));
	round (a, b, c, d, e, f2, k2, expand(w, 30));
	round (e, a, b, c, d, f2, k2, expand(w, 31));
	round (d, e, a, b, c, f2, k2, expand(w, 32));
	round (c, d, e, a, b, f2, k2, expand(w, 33));
	round (b, c, d, e, a, f2, k2, expand(w, 34));
	round (a, b, c, d, e, f2, k2, expand(w, 35));
	round (e, a, b, c, d, f2, k2, expand(w, 36));
	round (d, e, a, b, c, f2, k2, expand(w, 37));
	round (c, d, e, a, b, f2, k2, expand(w, 38));
	round (b, c, d, e, a, f2, k2, expand(w, 39));

	round (a, b, c, d, e, f3, k3, expand(w, 40));
	round (e, a, b, c, d, f3, k3, expand(w, 41));
	round (d, e, a, b, c, f3, k3, expand(w, 42));
	round (c, d, e, a, b, f3, k3, expand(w, 43));
	round (b, c, d, e, a, f3, k3, expand(w, 44));
	round (a, b, c, d, e, f3, k3, expand(w, 45));
	round (e, a, b, c, d, f3, k3, expand(w, 46));
	round (d, e, a, b, c, f3, k3, expand(w, 47));
	round (c, d, e, a, b, f3, k3, expand(w, 48));
	round (b, c, d, e, a, f3, k3, expand(w, 49));
	round (a, b, c, d, e, f3, k3, expand(w, 50));
	round (e, a, b, c, d, f3, k3, expand(w, 51));
	round (d, e, a, b, c, f3, k3, expand(w, 52));
	round (c, d, e, a, b, f3, k3, expand(w, 53));
	round (b, c, d, e, a, f3, k3, expand(w, 54));
	round (a, b, c, d, e, f3, k3, expand(w, 55));
	round (e, a, b, c, d, f3, k3, expand(w, 56));
	round (d, e, a, b, c, f3, k3, expand(w, 57));
	round (c, d, e, a, b, f3, k3, expand(w, 58));
	round (b, c, d, e, a, f3, k3, expand(w, 59));

	round (a, b, c, d, e, f4, k4, expand(w, 60));
	round (e, a, b, c, d, f4, k4, expand(w, 61));
	round (d, e, a, b, c, f4, k4, expand(w, 62));
	round (c, d, e, a, b, f4, k4, expand(w, 63));
	round (b, c, d, e, a, f4, k4, expand(w, 64));
	round (a, b, c, d, e, f4, k4, expand(w, 65));
	round (e, a, b, c, d, f4, k4, expand(w, 66));
	round (d, e, a, b, c, f4, k4, expand(w, 67));
	round (c, d, e, a, b, f4, k4, expand(w, 68));
	round (b, c, d, e, a, f4, k4, expand(w, 69));
	round (a, b, c, d, e, f4, k4, expand(w, 70));
	round (e, a, b, c, d, f4, k4, expand(w, 71));
	round (d, e, a, b, c, f4, k4, expand(w, 72));
	round (c, d, e, a, b, f4, k4, expand(w, 73));
	round (b, c, d, e, a, f4, k4, expand(w, 74));
	round (a, b, c, d, e, f4, k4, expand(w, 75));
	round (e, a, b, c, d, f4, k4, expand(w, 76));
	round (d, e, a, b, c, f4, k4, expand(w, 77));
	round (c, d, e, a, b, f4, k4, expand(w, 78));
	round (b, c, d, e, a, f4, k4, expand(w, 79));

	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;
}


void
sha1_update (SHA1Context *sha, const gchar *buf, guint32 count)
{
	guint32 tmp;
	guint32 len;

	tmp = sha->low;

	if ((sha->low += count << 3) < tmp)
		sha->high++;

	sha->high += count >> 29;

	len = (tmp >> 3) & 0x3F;

	if (len) {
		gchar *p = ((gchar *) sha->data) + len;

		len = SHA1_DATA_SIZE - len;

		if (count < len) {
			memcpy (p, buf, count);
			return;
		}

		memcpy (p, buf, len);
		sha1_to_be (sha->data, SHA1_DATA_SIZE);
		sha1_digest (sha->digest, sha->data);
		buf += len;
		count -= len;
	}

	while (count >= SHA1_DATA_SIZE) {
		memcpy (sha->data, buf, SHA1_DATA_SIZE);
		sha1_to_be (sha->data, SHA1_DATA_SIZE);
		sha1_digest (sha->digest, sha->data);
		buf += SHA1_DATA_SIZE;
		count -= SHA1_DATA_SIZE;
	}

	memcpy (sha->data, buf, count);
}


void
sha1_finish (SHA1Context *sha, gchar *out)
{
	gint count;
	guchar *data;

	count = (((gint) sha->low) >> 3) & 0x3F;

	data = ((guchar *) sha->data) + count;
	*data++ = 0x80;

	count = SHA1_DATA_SIZE - 1 - count;

	if (count < 8) {
		memset (data, 0, count);
		sha1_to_be (sha->data, SHA1_DATA_SIZE);
		sha1_digest (sha->digest, sha->data);
		memset (sha->data, 0, SHA1_DATA_SIZE - 8);
	} else
		memset (data, 0, count - 8);

	sha->data[14] = sha->high;
	sha->data[15] = sha->low;
	sha1_to_be (sha->data, SHA1_DATA_SIZE - 8);
	sha1_digest (sha->digest, sha->data);
	sha1_from_be (sha->digest, SHA1_DIGEST_SIZE);
  	memcpy (out, sha->digest, SHA1_DIGEST_SIZE);
	memset (sha, 0, sizeof(sha));
}
