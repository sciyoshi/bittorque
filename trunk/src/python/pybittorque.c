#include <pygobject.h>
 
void bittorque_register_classes (PyObject *d); 
extern PyMethodDef bittorque_functions[];
 
DL_EXPORT (void)
initbittorque ()
{
PyObject *m, *d;

	init_pygobject ();

	m = Py_InitModule ("bittorque", bittorque_functions);
	d = PyModule_GetDict (m);

	bittorque_register_classes (d);

	if (PyErr_Occurred ()) {
		Py_FatalError ("can't initialise module bittorque");
	}
}
