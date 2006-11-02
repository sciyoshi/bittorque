#ifndef __BITTORQUE_H__
#define __BITTORQUE_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#ifdef USE_GCONF
# include <gconf/gconf-client.h>
#endif

#include "../bittorque/bt-manager.h"
#include "../bittorque/bt-torrent.h"

typedef struct {
	GladeXML    *xml;
	GtkWidget   *window;
	GtkStatusIcon *icon;
	BtManager   *manager;

#ifdef USE_GCONF
	GConfClient *gconf;
#endif

} BittorqueApp;


extern BittorqueApp app;

#endif
