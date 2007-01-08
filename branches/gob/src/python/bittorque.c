/* -- THIS FILE IS GENERATED - DO NOT EDIT *//* -*- Mode: C; c-basic-offset: 4 -*- */

#include <Python.h>



#line 3 "bittorque.override"
#include <Python.h>
#include "pygobject.h"
#include "bt-manager.h"
#include "bt-utils.h"
#line 13 "bittorque.c"


/* ---------- types from other modules ---------- */
static PyTypeObject *_PyGObject_Type;
#define PyGObject_Type (*_PyGObject_Type)
static PyTypeObject *_PyMainContext_Type;
#define PyMainContext_Type (*_PyMainContext_Type)
static PyTypeObject *_PySource_Type;
#define PySource_Type (*_PySource_Type)


/* ---------- forward type declarations ---------- */
PyTypeObject G_GNUC_INTERNAL PyBtManager_Type;
PyTypeObject G_GNUC_INTERNAL PyBtPeer_Type;
PyTypeObject G_GNUC_INTERNAL PyBtTorrent_Type;

#line 30 "bittorque.c"



/* ----------- BtManager ----------- */

static int
_wrap_bt_manager_new(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType obj_type = pyg_type_from_object((PyObject *) self);
    GParameter params[1];
    PyObject *parsed_args[1] = {NULL, };
    char *arg_names[] = {"GMainContext*", NULL };
    char *prop_names[] = {"context", NULL };
    guint nparams, i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:bittorque.Manager.__init__" , arg_names , &parsed_args[0]))
        return -1;

    memset(params, 0, sizeof(GParameter)*1);
    if (!pyg_parse_constructor_args(obj_type, arg_names,
                                    prop_names, params, 
                                    &nparams, parsed_args))
        return -1;
    pygobject_constructv(self, nparams, params);
    for (i = 0; i < nparams; ++i)
        g_value_unset(&params[i].value);
    if (!self->obj) {
        PyErr_SetString(
            PyExc_RuntimeError, 
            "could not create bittorque.Manager object");
        return -1;
    }
    return 0;
}

static PyObject *
_wrap_bt_manager_accept_start(PyGObject *self)
{
    int ret;
    GError *error = NULL;

    
    ret = bt_manager_accept_start(BT_MANAGER(self->obj), &error);
    
    if (pyg_error_check(&error))
        return NULL;
    return PyBool_FromLong(ret);

}

static PyObject *
_wrap_bt_manager_accept_stop(PyGObject *self)
{
    
    bt_manager_accept_stop(BT_MANAGER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_manager_set_port(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "port", NULL };
    int port, ret;
    GError *error = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"i:BtManager.set_port", kwlist, &port))
        return NULL;
    
    ret = bt_manager_set_port(BT_MANAGER(self->obj), port, &error);
    
    if (pyg_error_check(&error))
        return NULL;
    return PyBool_FromLong(ret);

}

static PyObject *
_wrap_bt_manager_get_port(PyGObject *self)
{
    int ret;

    
    ret = bt_manager_get_port(BT_MANAGER(self->obj));
    
    return PyInt_FromLong(ret);
}

static PyObject *
_wrap_bt_manager_add_torrent(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "torrent", NULL };
    PyGObject *torrent;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:BtManager.add_torrent", kwlist, &PyBtTorrent_Type, &torrent))
        return NULL;
    
    bt_manager_add_torrent(BT_MANAGER(self->obj), BT_TORRENT(torrent->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_manager_get_torrent(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "infohash", NULL };
    char *infohash;
    BtTorrent *ret;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"s:BtManager.get_torrent", kwlist, &infohash))
        return NULL;
    
    ret = bt_manager_get_torrent(BT_MANAGER(self->obj), infohash);
    
    /* pygobject_new handles NULL checking */
    return pygobject_new((GObject *)ret);
}

static PyObject *
_wrap_bt_manager_get_torrent_string(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "infohash", NULL };
    char *infohash;
    BtTorrent *ret;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"s:BtManager.get_torrent_string", kwlist, &infohash))
        return NULL;
    
    ret = bt_manager_get_torrent_string(BT_MANAGER(self->obj), infohash);
    
    /* pygobject_new handles NULL checking */
    return pygobject_new((GObject *)ret);
}

static const PyMethodDef _PyBtManager_methods[] = {
    { "accept_start", (PyCFunction)_wrap_bt_manager_accept_start, METH_NOARGS,
      NULL },
    { "accept_stop", (PyCFunction)_wrap_bt_manager_accept_stop, METH_NOARGS,
      NULL },
    { "set_port", (PyCFunction)_wrap_bt_manager_set_port, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_port", (PyCFunction)_wrap_bt_manager_get_port, METH_NOARGS,
      NULL },
    { "add_torrent", (PyCFunction)_wrap_bt_manager_add_torrent, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_torrent", (PyCFunction)_wrap_bt_manager_get_torrent, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_torrent_string", (PyCFunction)_wrap_bt_manager_get_torrent_string, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyBtManager_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "bittorque.Manager",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyBtManager_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)_wrap_bt_manager_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- BtPeer ----------- */

 static int
_wrap_bt_peer_new(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType obj_type = pyg_type_from_object((PyObject *) self);
    GParameter params[4];
    PyObject *parsed_args[4] = {NULL, };
    char *arg_names[] = {"BtManager*", "BtTorrent*", "GTcpSocket*", "GInetAddr*", NULL };
    char *prop_names[] = {"manager", "torrent", "socket", "address", NULL };
    guint nparams, i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOO:bittorque.Peer.__init__" , arg_names , &parsed_args[0] , &parsed_args[1] , &parsed_args[2] , &parsed_args[3]))
        return -1;

    memset(params, 0, sizeof(GParameter)*4);
    if (!pyg_parse_constructor_args(obj_type, arg_names,
                                    prop_names, params, 
                                    &nparams, parsed_args))
        return -1;
    pygobject_constructv(self, nparams, params);
    for (i = 0; i < nparams; ++i)
        g_value_unset(&params[i].value);
    if (!self->obj) {
        PyErr_SetString(
            PyExc_RuntimeError, 
            "could not create bittorque.Peer object");
        return -1;
    }
    return 0;
}

static PyObject *
_wrap_bt_peer_set_choking(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "choked", NULL };
    int choked;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"i:BtPeer.set_choking", kwlist, &choked))
        return NULL;
    
    bt_peer_set_choking(BT_PEER(self->obj), choked);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_peer_set_interesting(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "choked", NULL };
    int choked;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"i:BtPeer.set_interesting", kwlist, &choked))
        return NULL;
    
    bt_peer_set_interesting(BT_PEER(self->obj), choked);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_peer_connect(PyGObject *self)
{
    
    bt_peer_connect(BT_PEER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_peer_disconnect(PyGObject *self)
{
    int ret;

    
    ret = bt_peer_disconnect(BT_PEER(self->obj));
    
    return PyBool_FromLong(ret);

}

static PyObject *
_wrap_bt_peer_send_keepalive(PyGObject *self)
{
    
    bt_peer_send_keepalive(BT_PEER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_peer_send_choke(PyGObject *self)
{
    
    bt_peer_send_choke(BT_PEER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_peer_send_unchoke(PyGObject *self)
{
    
    bt_peer_send_unchoke(BT_PEER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_peer_send_interested(PyGObject *self)
{
    
    bt_peer_send_interested(BT_PEER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_peer_send_uninterested(PyGObject *self)
{
    
    bt_peer_send_uninterested(BT_PEER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_peer_send_handshake(PyGObject *self)
{
    
    bt_peer_send_handshake(BT_PEER(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyBtPeer_methods[] = {
    { "set_choking", (PyCFunction)_wrap_bt_peer_set_choking, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "set_interesting", (PyCFunction)_wrap_bt_peer_set_interesting, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "connect", (PyCFunction)_wrap_bt_peer_connect, METH_NOARGS,
      NULL },
    { "disconnect", (PyCFunction)_wrap_bt_peer_disconnect, METH_NOARGS,
      NULL },
    { "send_keepalive", (PyCFunction)_wrap_bt_peer_send_keepalive, METH_NOARGS,
      NULL },
    { "send_choke", (PyCFunction)_wrap_bt_peer_send_choke, METH_NOARGS,
      NULL },
    { "send_unchoke", (PyCFunction)_wrap_bt_peer_send_unchoke, METH_NOARGS,
      NULL },
    { "send_interested", (PyCFunction)_wrap_bt_peer_send_interested, METH_NOARGS,
      NULL },
    { "send_uninterested", (PyCFunction)_wrap_bt_peer_send_uninterested, METH_NOARGS,
      NULL },
    { "send_handshake", (PyCFunction)_wrap_bt_peer_send_handshake, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyBtPeer_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "bittorque.Peer",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyBtPeer_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)_wrap_bt_peer_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- BtTorrent ----------- */

 static int
_wrap_bt_torrent_new(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    GType obj_type = pyg_type_from_object((PyObject *) self);
    GParameter params[2];
    PyObject *parsed_args[2] = {NULL, };
    char *arg_names[] = {"BtManager", "const-gchar*", NULL };
    char *prop_names[] = {"manager", "filename", NULL };
    guint nparams, i;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO:bittorque.Torrent.__init__" , arg_names , &parsed_args[0] , &parsed_args[1]))
        return -1;

    memset(params, 0, sizeof(GParameter)*2);
    if (!pyg_parse_constructor_args(obj_type, arg_names,
                                    prop_names, params, 
                                    &nparams, parsed_args))
        return -1;
    pygobject_constructv(self, nparams, params);
    for (i = 0; i < nparams; ++i)
        g_value_unset(&params[i].value);
    if (!self->obj) {
        PyErr_SetString(
            PyExc_RuntimeError, 
            "could not create bittorque.Torrent object");
        return -1;
    }
    return 0;
}

static PyObject *
_wrap_bt_torrent_set_location(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "location", NULL };
    char *location;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"s:BtTorrent.set_location", kwlist, &location))
        return NULL;
    
    bt_torrent_set_location(BT_TORRENT(self->obj), location);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_torrent_start_downloading(PyGObject *self)
{
    int ret;

    
    ret = bt_torrent_start_downloading(BT_TORRENT(self->obj));
    
    return PyBool_FromLong(ret);

}

static PyObject *
_wrap_bt_torrent_announce(PyGObject *self)
{
    int ret;

    
    ret = bt_torrent_announce(BT_TORRENT(self->obj));
    
    return PyBool_FromLong(ret);

}

static PyObject *
_wrap_bt_torrent_announce_stop(PyGObject *self)
{
    int ret;

    
    ret = bt_torrent_announce_stop(BT_TORRENT(self->obj));
    
    return PyBool_FromLong(ret);

}

static PyObject *
_wrap_bt_torrent_add_peer(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "peer", NULL };
    PyGObject *peer;
    int ret;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:BtTorrent.add_peer", kwlist, &PyBtPeer_Type, &peer))
        return NULL;
    
    ret = bt_torrent_add_peer(BT_TORRENT(self->obj), BT_PEER(peer->obj));
    
    return PyBool_FromLong(ret);

}

static const PyMethodDef _PyBtTorrent_methods[] = {
    { "set_location", (PyCFunction)_wrap_bt_torrent_set_location, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "start_downloading", (PyCFunction)_wrap_bt_torrent_start_downloading, METH_NOARGS,
      NULL },
    { "announce", (PyCFunction)_wrap_bt_torrent_announce, METH_NOARGS,
      NULL },
    { "announce_stop", (PyCFunction)_wrap_bt_torrent_announce_stop, METH_NOARGS,
      NULL },
    { "add_peer", (PyCFunction)_wrap_bt_torrent_add_peer, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyBtTorrent_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "bittorque.Torrent",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyBtTorrent_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)_wrap_bt_torrent_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- functions ----------- */

static PyObject *
_wrap_bt_create_peer_id(PyObject *self)
{
    gchar *ret;

    
    ret = bt_create_peer_id();
    
    if (ret) {
        PyObject *py_ret = PyString_FromString(ret);
        g_free(ret);
        return py_ret;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_url_encode(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "string", "size", NULL };
    char *string;
    gsize size;
    gchar *ret;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"sk:bt_url_encode", kwlist, &string, &size))
        return NULL;
    
    ret = bt_url_encode(string, size);
    
    if (ret) {
        PyObject *py_ret = PyString_FromString(ret);
        g_free(ret);
        return py_ret;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_hash_to_string(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "hash", NULL };
    char *hash;
    gchar *ret;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"s:bt_hash_to_string", kwlist, &hash))
        return NULL;
    
    ret = bt_hash_to_string(hash);
    
    if (ret) {
        PyObject *py_ret = PyString_FromString(ret);
        g_free(ret);
        return py_ret;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_bt_client_name_from_id(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "id", NULL };
    char *id;
    gchar *ret;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"s:bt_client_name_from_id", kwlist, &id))
        return NULL;
    
    ret = bt_client_name_from_id(id);
    
    if (ret) {
        PyObject *py_ret = PyString_FromString(ret);
        g_free(ret);
        return py_ret;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

const PyMethodDef bittorque_functions[] = {
    { "bt_create_peer_id", (PyCFunction)_wrap_bt_create_peer_id, METH_NOARGS,
      NULL },
    { "bt_url_encode", (PyCFunction)_wrap_bt_url_encode, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "bt_hash_to_string", (PyCFunction)_wrap_bt_hash_to_string, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "bt_client_name_from_id", (PyCFunction)_wrap_bt_client_name_from_id, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { NULL, NULL, 0, NULL }
};


/* ----------- enums and flags ----------- */

void
bittorque_add_constants(PyObject *module, const gchar *strip_prefix)
{
  pyg_enum_add(module, "PeerStatus", strip_prefix, BT_TYPE_PEER_STATUS);
  pyg_enum_add(module, "TorrentPriority", strip_prefix, BT_TYPE_TORRENT_PRIORITY);
  pyg_enum_add(module, "Error", strip_prefix, BT_TYPE_ERROR);

  if (PyErr_Occurred())
    PyErr_Print();
}

/* initialise stuff extension classes */
void
bittorque_register_classes(PyObject *d)
{
    PyObject *module;

    if ((module = PyImport_ImportModule("gobject")) != NULL) {
        _PyGObject_Type = (PyTypeObject *)PyObject_GetAttrString(module, "GObject");
        if (_PyGObject_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name GObject from gobject");
            return ;
        }
        _PyMainContext_Type = (PyTypeObject *)PyObject_GetAttrString(module, "MainContext");
        if (_PyMainContext_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name MainContext from gobject");
            return ;
        }
        _PySource_Type = (PyTypeObject *)PyObject_GetAttrString(module, "Source");
        if (_PySource_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name Source from gobject");
            return ;
        }
    } else {
        PyErr_SetString(PyExc_ImportError,
            "could not import gobject");
        return ;
    }


#line 748 "bittorque.c"
    pygobject_register_class(d, "BtManager", BT_TYPE_MANAGER, &PyBtManager_Type, Py_BuildValue("(O)", &PyGObject_Type));
    pyg_set_object_has_new_constructor(BT_TYPE_MANAGER);
    pygobject_register_class(d, "BtPeer", BT_TYPE_PEER, &PyBtPeer_Type, Py_BuildValue("(O)", &PyGObject_Type));
    pyg_set_object_has_new_constructor(BT_TYPE_PEER);
    pygobject_register_class(d, "BtTorrent", BT_TYPE_TORRENT, &PyBtTorrent_Type, Py_BuildValue("(O)", &PyGObject_Type));
    pyg_set_object_has_new_constructor(BT_TYPE_TORRENT);
}
