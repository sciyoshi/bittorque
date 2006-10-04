#include <glib.h>
#include <glib-object.h>

#include "torque-torrent-file.h"
#include "torque-utils.h"
#include "torque-torrent.h"

struct _TorqueTorrentFile{
	GObject parent;

	TorqueTorrent *torrent;

	gchar *name;
	gchar *location;

	gsize size;
};

struct _TorqueTorrentFileClass {
	GObjectClass parent;
};

G_DEFINE_TYPE (TorqueTorrentFile, torque_torrent_file, G_TYPE_OBJECT)

static void
torque_torrent_file_init (TorqueTorrentFile *self)
{
	self->name = NULL;
	self->location = NULL;
	return;
}

static void
torque_torrent_file_class_init (TorqueTorrentFileClass *klass G_GNUC_UNUSED)
{
	return;
}

TorqueTorrentFile *
torque_torrent_file_new (TorqueTorrent *torrent, gchar *name, gsize size)
{
	TorqueTorrentFile *file = TORQUE_TORRENT_FILE (g_object_new (TORQUE_TYPE_TORRENT_FILE, NULL));

	file->name = g_strdup (name);
	file->torrent = g_object_ref (torrent);
	file->size = size;

	return file;
}
