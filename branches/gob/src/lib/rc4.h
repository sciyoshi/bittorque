/**
 * rc4.h
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
