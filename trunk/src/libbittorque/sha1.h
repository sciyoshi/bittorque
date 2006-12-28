/**
 * sha1.h
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
