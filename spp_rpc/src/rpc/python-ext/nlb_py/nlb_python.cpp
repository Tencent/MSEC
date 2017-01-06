
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


#include "nlbapi.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Python.h>

static void ip2str(uint32_t ip, char * str)
{
    unsigned char * p = (unsigned char*)&ip;

    sprintf(str, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
}

static uint32_t str2ip(const char * str)
{
    struct in_addr addr;
    inet_aton(str, &addr);

    return addr.s_addr;
}

PyObject* wrap_getroute(PyObject* self, PyObject* args)
{
  char* service;
  int result;
  struct routeid rid;
  char ip_str[128];
  char type_str[10];

  ip_str[0] = '\0';
  type_str[0] = '\0';
  if (! PyArg_ParseTuple(args, "s", &service))
    return NULL;
  result = getroutebyname(service, &rid);
  if (result == 0)
  {
      ip2str(rid.ip, ip_str); 
      switch (rid.type) {
	  case NLB_PORT_TYPE_ALL:
	    snprintf(type_str, sizeof(type_str), "all");
		break;
	  case NLB_PORT_TYPE_UDP:
	    snprintf(type_str, sizeof(type_str), "udp");
		break;
	  case NLB_PORT_TYPE_TCP:
	  default:
	    snprintf(type_str, sizeof(type_str), "tcp");
		break;
	  }

      return Py_BuildValue("{s:s,s:i,s:s}",
              "ip", ip_str, "port", rid.port, "type", type_str);
  }

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject* wrap_updateroute(PyObject* self, PyObject* args)
{
  char* service;
  char* ip_str;
  int failed;
  int timecost;
  int result;

  if (! PyArg_ParseTuple(args, "ssii", &service, &ip_str, &failed, &timecost))
    return NULL;
	
  uint32_t ip = str2ip(ip_str);
  result = updateroute(service, ip, failed, timecost);
  return Py_BuildValue("i", result);
}

static PyMethodDef nlbMethods[] =
{
  {"getroutebyname", wrap_getroute, METH_VARARGS, "get server address"},
  {"updateroute", wrap_updateroute, METH_VARARGS, "update server status"},
  {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef nlbModule = {
    PyModuleDef_HEAD_INIT,
    "nlb_py",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,   /* size of per-interpreter state of the module,
             or -1 if the module keeps state in global variables. */
    nlbMethods
};

// The initialization function must be named PyInit_name()
extern "C" PyMODINIT_FUNC PyInit_nlb_py(void)
{
    return PyModule_Create(&nlbModule);
}

#else

extern "C" PyMODINIT_FUNC initnlb_py()
{
  PyObject* m;
  m = Py_InitModule("nlb_py", nlbMethods);
}

#endif

