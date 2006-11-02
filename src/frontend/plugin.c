#include <glib.h>
#include <gmodule.h>

#include "bittorque.h"

gboolean
bt_plugin_load (gchar *name, GError **error)
{
	GModule *module;

	gpointer (*init) (BtApp *bittorque);

	gpointer data;

	module = g_module_open (name, G_MODULE_BIND_LAZY);

	if (!module) {
		g_set_error (error, BITTORQUE_ERROR, BITTORQUE_ERROR_PLUGIN, "%s", g_module_error ());
		return FALSE;
	}

	if (!g_module_find_symbol (module, "init", (gpointer) &init)) {
		g_set_error (error, BITTORQUE_ERROR, BITTORQUE_ERROR_PLUGIN, "%s", name, g_module_error ())) {
		if (!g_module_close (module))
			g_warning ("%s: %s", name, g_module_error ());
		return FALSE;
	}

	data = init (bittorque);

	return TRUE;
}
