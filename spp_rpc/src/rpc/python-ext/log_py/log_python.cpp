
/**
 * Tencent is pleased to support the open source community by making MSEC available.
 *
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the GNU General Public License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. You may 
 * obtain a copy of the License at
 *
 *     https://opensource.org/licenses/GPL-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the 
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */


#include "logsys_api.h"
#include <iostream>  
#include <sstream>  
#include <Python.h>  
#include <structmember.h>  
using namespace std;
using namespace msec;

typedef struct _LogApiWrapper
{
	PyObject_HEAD      // == PyObject ob_base;  定义一个PyObject对象.  
	PyObject *header_map;
	LogsysApi*  inner_log_api;
} LogApiWrapper;

static void
LogApi_dealloc(LogApiWrapper* self)
{
    Py_XDECREF(self->header_map);
    //delete  inner_log_api;
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *
LogApi_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    LogApiWrapper *self;

    self = (LogApiWrapper *)type->tp_alloc(type, 0);
    if (self != NULL) {
        self->header_map = PyDict_New();
        if (self->header_map == NULL)
        {
            Py_DECREF(self);
            return NULL;
        }

        self->inner_log_api = LogsysApi::GetInstance();
        if (self->inner_log_api == NULL)
        {
            Py_DECREF(self);
            return NULL;
        }
    }

    return (PyObject *)self;
}

static void
LogApi_init(LogApiWrapper *self, PyObject *args)
{
	char* conf;
	int result = -1;

	if (!PyArg_ParseTuple(args, "s", &conf))
		return;

	result = self->inner_log_api->Init(conf);
    return;
}

static PyMemberDef LogApi_members[] = {
    { "header_map", T_OBJECT_EX, offsetof(LogApiWrapper, header_map), 0, "header map" },
    { NULL }  /* Sentinel */
};

static PyObject* LogApi_set(LogApiWrapper* self, PyObject* args)
{
    char* header_name;
    char* header_value;
    int result = -1;

    if (!PyArg_ParseTuple(args, "ss", &header_name, &header_value))
        return NULL;

    PyObject *value_obj = PyString_FromString(header_value);
    if (value_obj != NULL)
    {
        result = PyDict_SetItemString(self->header_map, header_name, value_obj);
		Py_DECREF(value_obj);
    }
    return Py_BuildValue("i", result);
}

static PyObject* LogApi_get(LogApiWrapper* self)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    int result = 0;
    while (PyDict_Next(self->header_map, &pos, &key, &value)) {
        char* key_str = PyString_AsString(key);
        char* value_str = PyString_AsString(value);
        if (key_str == NULL || value_str == NULL)
        {
            result = -1;
            break;
        }
        printf("key: %s, value: %s\n", key_str, value_str);
    }

    return Py_BuildValue("i", result);
}

static PyObject* LogApi_log(LogApiWrapper* self, PyObject* args)
{
	char* log_body;
	int result = -1;
	PyObject *key, *value;
	Py_ssize_t pos = 0;

	if (!PyArg_ParseTuple(args, "s", &log_body))
		return NULL;

	std::map<std::string, std::string>  kv_map;
	while (PyDict_Next(self->header_map, &pos, &key, &value)) {
		char* key_str = PyString_AsString(key);
		char* value_str = PyString_AsString(value);
		if (key_str == NULL || value_str == NULL)
		{
			result = -1;
			break;
		}
		kv_map[key_str] = value_str;
	}
	result = self->inner_log_api->Log(LogsysApi::FATAL, kv_map, log_body);
	return Py_BuildValue("i", result);
}

static PyMethodDef LogApi_methods[] = {
    {"set", (PyCFunction)LogApi_set, METH_VARARGS, "set header and value"},
    {"get", (PyCFunction)LogApi_get, METH_NOARGS, "get header and value" },
	{"log", (PyCFunction)LogApi_log, METH_VARARGS, "write log" },
    { NULL }  /* Sentinel */
};

static PyTypeObject LogApiType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "LogApiWrapper.LogApiWrapper",             /*tp_name*/
    sizeof(LogApiWrapper),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)LogApi_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "LogApiWrapper objects",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    LogApi_methods,             /* tp_methods */
    LogApi_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
	(initproc)LogApi_init,      /* tp_init */
    0,                         /* tp_alloc */
    LogApi_new,                 /* tp_new */
};

static PyMethodDef module_methods[] = {
    { NULL }  /* Sentinel */
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef logModule = {
    PyModuleDef_HEAD_INIT,
    "msec_log",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,   /* size of per-interpreter state of the module,
             or -1 if the module keeps state in global variables. */
    module_methods
};

// The initialization function must be named PyInit_name()
extern "C" PyMODINIT_FUNC PyInit_msec_log(void)
{
    PyObject* m;

    if (PyType_Ready(&LogApiType) < 0)
        return;

    m = PyModule_Create(&logModule);
    if (m == NULL)
        return;

    Py_INCREF(&LogApiType);
    PyModule_AddObject(m, "LogApiWrapper", (PyObject *)&LogApiType);
}

#else

extern "C" PyMODINIT_FUNC  initmsec_log(void)
{
    PyObject* m;

    if (PyType_Ready(&LogApiType) < 0)
        return;

    m = Py_InitModule3("msec_log", module_methods,
        "Example module that creates an extension type.");

    if (m == NULL)
        return;

    Py_INCREF(&LogApiType);
    PyModule_AddObject(m, "LogApiWrapper", (PyObject *)&LogApiType);
}

#endif

