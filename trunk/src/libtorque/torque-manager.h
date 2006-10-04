#ifndef __TORQUE_MANAGER_H__
#define __TORQUE_MANAGER_H__

#include <glib.h>
#include <glib-object.h>

#include "torque-torrent.h"

#define TORQUE_TYPE_MANAGER (torque_manager_get_type ())
#define TORQUE_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TORQUE_TYPE_MANAGER, TorqueManager))

typedef struct _TorqueManager TorqueManager;
typedef struct _TorqueManagerClass TorqueManagerClass;

GType torque_manager_get_type ();

gboolean torque_manager_add_torrent (TorqueManager *manager, TorqueTorrent *torrent, GError **error);
gboolean torque_manager_remove_torrent (TorqueManager *manager, TorqueTorrent *torrent, GError **error);

#endif
