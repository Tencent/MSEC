
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


#include <stdio.h>
#include <Python.h>
#include <string.h>
#include <stdint.h>
#include "configini.h"
#include "srpc_cintf.h"

PyObject* warp_py_get_config(PyObject* self, PyObject* args)
{
    char *key;
    char *filename;
    char *session;

    if (! PyArg_ParseTuple(args, "sss", &filename, &session, &key))
        return NULL;

    string key_str(key);
    string val_str;
    string file_str(filename);
    string session_str(session);

    int ret = GetConfig(file_str, session_str, key_str, val_str);
    if (ret != CONF_RET_OK)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    return Py_BuildValue("s", val_str.c_str());
}

PyObject* warp_py_log_set_option_str(PyObject* self, PyObject* args)
{
    char *key;
    char *val;

    if (! PyArg_ParseTuple(args, "ss", &key, &val))
        return NULL;

    set_log_option_str(key, val);

    Py_INCREF(Py_None);

    return Py_None;
}

PyObject* warp_py_log_set_option_int(PyObject* self, PyObject* args)
{
    char *key;
    int   val;

    if (! PyArg_ParseTuple(args, "si", &key, &val))
        return NULL;

    set_log_option_long(key, (int64_t)val);

    Py_INCREF(Py_None);

    return Py_None;
}

PyObject* warp_py_log_err(PyObject* self, PyObject* args)
{
    char *str;

    if (! PyArg_ParseTuple(args, "s", &str))
        return NULL;

    msec_log_error(str);

    Py_INCREF(Py_None);

    return Py_None;
}

PyObject* warp_py_log_info(PyObject* self, PyObject* args)
{
    char *str;

    if (! PyArg_ParseTuple(args, "s", &str))
        return NULL;

    msec_log_info(str);

    Py_INCREF(Py_None);

    return Py_None;
}

PyObject* warp_py_log_debug(PyObject* self, PyObject* args)
{
    char *str;

    if (! PyArg_ParseTuple(args, "s", &str))
        return NULL;

    msec_log_debug(str);

    Py_INCREF(Py_None);

    return Py_None;
}

PyObject* warp_py_log_fatal(PyObject* self, PyObject* args)
{
    char *str;

    if (! PyArg_ParseTuple(args, "s", &str))
        return NULL;

    msec_log_fatal(str);

    Py_INCREF(Py_None);

    return Py_None;
}

static PyMethodDef logMethods[] =
{
  {"nglog_set_option_str", warp_py_log_set_option_str, METH_VARARGS, "set string log option"},
  {"nglog_set_option_int", warp_py_log_set_option_int, METH_VARARGS, "set interger log option"},
  {"nglog_error",          warp_py_log_err,            METH_VARARGS, "log error"},
  {"nglog_info",           warp_py_log_info,           METH_VARARGS, "log info"},
  {"nglog_debug",          warp_py_log_debug,          METH_VARARGS, "log debug"},
  {"nglog_fatal",          warp_py_log_fatal,          METH_VARARGS, "log fatal"},
  {"get_config",           warp_py_get_config,         METH_VARARGS, "get config"},
  {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef logModule = {
    PyModuleDef_HEAD_INIT,
    "log_py",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,   /* size of per-interpreter state of the module,
             or -1 if the module keeps state in global variables. */
    logMethods
};

// The initialization function must be named PyInit_name()
extern "C" PyMODINIT_FUNC PyInit_log_py(void)
{
    return PyModule_Create(&logModule);
}

#else

extern "C" PyMODINIT_FUNC initlog_py()
{
    PyObject* m;
    m = Py_InitModule("log_py", logMethods);
}

#endif

