#ifndef __BITTORQUE_H__
#define __BITTORQUE_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>

#include "torque-utils.h"

typedef struct {
	GladeXML *xml;
	GConfClient *gconf;
} TorqueApp;

extern TorqueApp *app;

#endif
