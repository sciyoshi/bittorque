#include <pygobject.h>
 
void torque_register_classes (PyObject *d); 
extern PyMethodDef torque_functions[];

DL_EXPORT (void)
inittorque (void)
{
	PyObject *m, *d;

	init_pygobject ();
 
	m = Py_InitModule ("torque", torque_functions);
	d = PyModule_GetDict (m);

	torque_register_classes (d);
 
	if (PyErr_Occurred ()) {
		Py_FatalError ("can't initialise module bittorque");
	}
}

