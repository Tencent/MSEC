
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


#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "monitor_client.h"
#include <Python.h>

static char *g_py_service_name = NULL;

static int service_name(char *buff, int len)
{
    char *name_pos;
    char *pos;
    char path[256];

    if (NULL == buff || len < 3) {
        return -1;
    }

    getcwd(path, sizeof(path));
    if (strncmp(path, "/msec/", 6)) {
        return -2;
    }

    name_pos = path + 6;
    pos = index(name_pos, '/');
    if (NULL == pos) {
        return -3;
    }

    *pos = '.';

    pos = index(pos, '/');
    if (NULL == pos) {
        return -4;
    }

    *pos++ = '\0';

    if (strncmp(pos, "bin", 4)) {
        return -5;
    }

    if (strlen(name_pos) >= len) {
        return -6;
    }

    strncpy(buff, name_pos, len);

    return 0;
}

static int msec_service_name(char *buff, int len)
{
    const char *default_service_name = "default.default";
    if (NULL == buff || len < (strlen(default_service_name) + 1)) {
        return -1;
    }

    if (service_name(buff, len) < 0) {
        strncpy(buff, default_service_name, len);
    }

    return 0;
}

static void init_servicename(void)
{
    char *servicename;
    servicename = (char *)malloc(256);
    memset(servicename, 0, 256);
    msec_service_name(servicename, 256);
    g_py_service_name = servicename;
}

static inline char *get_servicename(void)
{
    return g_py_service_name;
}

PyObject* attr_add(PyObject* self, PyObject* args)
{
  char attr_str[256];
  char* attr;
  int value, result;

  if (! PyArg_ParseTuple(args, "si", &attr, &value))
    return NULL;
  snprintf(attr_str, sizeof(attr_str), "usr.%s", attr);
  result = Monitor_Add(get_servicename(), attr_str, value);
  return Py_BuildValue("i", result);
}

PyObject* rpc_add(PyObject* self, PyObject* args)
{
  char attr_str[256];
  char* attr;
  int value, result;

  if (! PyArg_ParseTuple(args, "si", &attr, &value))
    return NULL;
  snprintf(attr_str, sizeof(attr_str), "frm.rpc %s", attr);
  result = Monitor_Add(get_servicename(), attr_str, value);
  return Py_BuildValue("i", result);
}


PyObject* attr_set(PyObject* self, PyObject* args)
{
  char attr_str[256];
  char* attr;
  int value, result;

  if (! PyArg_ParseTuple(args, "si", &attr, &value))
    return NULL;
  snprintf(attr_str, sizeof(attr_str), "usr.%s", attr);
  result = Monitor_Set(get_servicename(), attr_str, value);
  return Py_BuildValue("i", result);
}

PyObject* rpc_set(PyObject* self, PyObject* args)
{
  char attr_str[256];
  char* attr;
  int value, result;

  if (! PyArg_ParseTuple(args, "si", &attr, &value))
    return NULL;
  snprintf(attr_str, sizeof(attr_str), "frm.rpc %s", attr);
  result = Monitor_Set(get_servicename(), attr_str, value);
  return Py_BuildValue("i", result);
}

PyObject* monitor_add(PyObject* self, PyObject* args)
{
  char *attr;
  char *service;
  int value, result;

  if (! PyArg_ParseTuple(args, "ssi", &service, &attr, &value))
    return NULL;
  result = Monitor_Add(service, attr, value);
  return Py_BuildValue("i", result);
}


PyObject* monitor_set(PyObject* self, PyObject* args)
{
  char *service;
  char *attr;
  int   value, result;

  if (! PyArg_ParseTuple(args, "ssi", &service, &attr, &value))
    return NULL;
  result = Monitor_Set(service, attr, value);
  return Py_BuildValue("i", result);
}


static PyMethodDef monitorMethods[] =
{
  {"attr_report", attr_add, METH_VARARGS, "add report"},
  {"attr_set", attr_set, METH_VARARGS, "set report"},
  {"rpc_report", rpc_add, METH_VARARGS, "add rpc report"},
  {"rpc_set", rpc_set, METH_VARARGS, "set rpc report"},
  {"monitor_add", monitor_add, METH_VARARGS, "add monitor report"},
  {"monitor_set", monitor_set, METH_VARARGS, "set monitor report"},
  {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef monitorModule = {
    PyModuleDef_HEAD_INIT,
    "monitor_py",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,   /* size of per-interpreter state of the module,
             or -1 if the module keeps state in global variables. */
    monitorMethods
};

// The initialization function must be named PyInit_name()
extern "C" PyMODINIT_FUNC PyInit_monitor_py(void)
{
    init_servicename();
    return PyModule_Create(&monitorModule);
}

#else

extern "C" PyMODINIT_FUNC initmonitor_py()
{
  PyObject* m;
  init_servicename();
  m = Py_InitModule("monitor_py", monitorMethods);
}

#endif

