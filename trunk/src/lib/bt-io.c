/**
 * bt-manager.c
 *
 * Copyright 2007 Samuel Cormier-Iijima <sciyoshi@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <glib/giochannel.h>
#include <glib/gstdio.h>
#include <glib/gfileutils.h>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "bt-io.h"
#include "bt-utils.h"
#include "sha1.h"

enum {
	BT_IO_PROPERTY_TORRENT = 1
};

struct _BtIO {
	GObject parent;

	/* a weak reference to the torrent associated with this io object */
	BtTorrent *torrent;

	/* a hashtable mapping file path -> iochannel*/
	GHashTable *channels;	
};

struct _BtIOClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (BtIO, bt_io, G_TYPE_OBJECT)

static const BtTorrentFile *
bt_io_get_file_for_piece (BtIO *io, guint piece)
{
	gsize byte_pos;
	gsize byte_offset;
	guint index;
	const BtTorrentFile *file = NULL;

	g_return_val_if_fail (piece < bt_torrent_get_num_pieces (io->torrent), NULL);

	index = 0;

	byte_pos = piece * bt_torrent_get_piece_length (io->torrent);

	while (index < bt_torrent_get_num_files (io->torrent))
	{
		file = bt_torrent_get_file (io->torrent, index);

		if (file == NULL)
			break;

		byte_offset = file->size + file->offset;

		if (byte_offset >= byte_pos)
			break;
		
		index++;
	}

	g_assert (index < bt_torrent_get_num_files (io->torrent));

	return file;
}

static int
bt_io_open_file (BtIO *io, const gchar* path, gsize size)
{
	int fd;
	gchar* dir;

	if (!g_file_test (path, G_FILE_TEST_EXISTS))
	{
		dir = g_path_get_dirname (path);

		if (!g_file_test (dir, G_FILE_TEST_EXISTS))
			g_mkdir_with_parents (dir, 0755);

		g_free (dir);
	}

	fd = g_open (path, O_RDWR | O_CREAT, 0644);

	if (fd != -1)
		ftruncate (fd, size);

	return fd;
}

static GIOChannel *
bt_io_get_io_channel (BtIO *io, guint piece, guint begin, guint len)
{
	guint32 piece_length;
	gchar *path;
	GIOChannel *channel;
	const BtTorrentFile *file;

	g_return_val_if_fail (BT_IS_IO (io), NULL);

	file = bt_io_get_file_for_piece (io, piece);

	g_assert (file != NULL);

	piece_length = bt_torrent_get_piece_length_extended (io->torrent, piece);
	g_return_val_if_fail (piece * piece_length + begin + len <= file->size, NULL);

	// TODO: implement save path for torrents
	// path = g_build_filename (bt_torrent_get_path (io->torrent), file.name, NULL);
	path = g_build_filename ("/tmp", file->name, NULL);

	channel = g_hash_table_lookup (io->channels, path);

	if (channel == NULL)
	{
		// FIXME: implement some system to limit number of open channels

		int fd = bt_io_open_file (io, path, file->size);

		g_return_val_if_fail (fd != -1, NULL);

		channel = g_io_channel_unix_new (fd);

		g_io_channel_set_encoding (channel, NULL, NULL);
		g_io_channel_set_close_on_unref (channel, TRUE);

		g_hash_table_insert (io->channels, g_strdup (path), channel);
	}

	g_free (path);

	return channel;
}

/**
 * bt_io_write:
 * @io: the io object
 * @piece: the piece index
 * @begin: byte offset in @piece
 * @len: number of bytes to write
 * @data: actual bytes to write
 *
 * Writes @len bytes from @data to the file specified by @piece.
 */
void
bt_io_write (BtIO *io, guint piece, guint begin, guint len, const gchar* data)
{
	gint64 offset;
	GIOChannel *channel;

	g_return_if_fail (BT_IS_IO (io));

	channel = bt_io_get_io_channel (io, piece, begin, len);

	g_return_if_fail (channel != NULL);

	// FIXME: handle iochannel errors below

	offset = begin + piece * bt_torrent_get_piece_length (io->torrent);

	g_io_channel_seek_position (channel, offset, G_SEEK_SET, NULL);
	g_io_channel_write_chars (channel, data, len, NULL, NULL);

	//g_io_channel_flush (channel, NULL);
}

gchar *
bt_io_read (BtIO *io, guint piece, guint begin, guint len)
{
	gchar* data;
	gsize read;
	gint64 offset;
	GIOChannel *channel;

	g_return_val_if_fail (BT_IS_IO (io), NULL);

	channel = bt_io_get_io_channel (io, piece, begin, len);

	g_return_val_if_fail (channel != NULL, NULL);

	data = g_malloc (len);

	// FIXME: handle iochannel errors below

	offset = begin + piece * bt_torrent_get_piece_length (io->torrent);

	g_io_channel_seek_position (channel, offset, G_SEEK_SET, NULL);
	g_io_channel_read_chars (channel, data, len, &read, NULL);

	return g_memdup (data, read);
}

/**
 * bt_io_check_piece_hash:
 * @io: the io object
 * @piece: the piece index
 *
 * Checks the infohash of the specified @piece.
 *
 * Returns: TRUE if succesfull, otherwise FALSE.
 */
gboolean
bt_io_check_piece_hash (BtIO *io, guint piece)
{
	SHA1Context  sha;
	gchar *hash;
	gchar *data;
	guint32 len;
	gboolean ret;

	g_return_val_if_fail (BT_IS_IO (io), FALSE);

	g_return_val_if_fail (piece < bt_torrent_get_num_pieces (io->torrent), FALSE);

	hash = g_malloc (20);

	len = bt_torrent_get_piece_length_extended (io->torrent, piece);
	data = bt_io_read (io, piece, 0, len);

	g_return_val_if_fail (data != NULL, FALSE);

	sha1_init (&sha);
	sha1_update (&sha, data, len);
	sha1_finish (&sha, hash);

	g_free (data);

	ret = !strncmp (bt_torrent_get_piece_hash (io->torrent, piece), hash, 20);

	g_free (hash);

	return ret;
}

static void
bt_io_clear_channels (gpointer key G_GNUC_UNUSED, gpointer value, gpointer user_data G_GNUC_UNUSED)
{
	GIOChannel* channel = (GIOChannel*)value;

	g_io_channel_flush (channel, NULL);
	g_io_channel_unref (channel);
}

static void
bt_io_dispose (GObject *object)
{
	BtIO *self = BT_IO (object);

	g_hash_table_foreach (self->channels, &bt_io_clear_channels, NULL);
	g_hash_table_unref (self->channels);

	self->channels = NULL;

	G_OBJECT_CLASS (bt_io_parent_class)->dispose (object);
}

static void
bt_io_finalize (GObject *object)
{
	G_OBJECT_CLASS (bt_io_parent_class)->finalize (object);
}

static void
bt_io_get_property (GObject *object, guint property, GValue *value, GParamSpec *pspec)
{
	BtIO *self = BT_IO (object);
	
	switch (property) {
	case BT_IO_PROPERTY_TORRENT:
		// FIXME: this might leave a dangling pointer in value
		g_value_set_pointer (value, self->torrent);
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
}

static void
bt_io_set_property (GObject *object, guint property, const GValue *value, GParamSpec *pspec)
{
	BtIO *self = BT_IO (object);

	switch (property) {
	case BT_IO_PROPERTY_TORRENT:
		bt_remove_weak_pointer (G_OBJECT (self->torrent), (gpointer)&self->torrent);
		self->torrent = BT_TORRENT (g_value_get_pointer (value));
		bt_add_weak_pointer (G_OBJECT (self->torrent), (gpointer)&self->torrent);
		break;
	
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property, pspec);
	}
	
	return;
}

static void
bt_io_init (BtIO *io)
{
	io->torrent = NULL;

	io->channels = g_hash_table_new (g_str_hash, g_str_equal);

	return;
}

static void
bt_io_class_init (BtIOClass *io_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (io_class);
	GParamSpec *pspec;
	
	object_class->dispose = bt_io_dispose;
	object_class->finalize = bt_io_finalize;
	object_class->get_property = bt_io_get_property;
	object_class->set_property = bt_io_set_property;

	/**
	 * BtIO:torrent:
	 *
	 * The #BtTorrent that this io object works for.
	 */
	pspec = g_param_spec_pointer ("torrent",
	                             "the torrent this io channel is serving",
	                             "The torrent that this io channel is working data for.",
	                             G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK);
	
	g_object_class_install_property (object_class, BT_IO_PROPERTY_TORRENT, pspec);

	return;
}
