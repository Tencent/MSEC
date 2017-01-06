
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


#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "spp_version.h"
#include "tlog.h"
#include "tstat.h"
#include "tcommu.h"
#include "serverbase.h"
#include "monitor.h"
#include "mt_incl.h"
#include "SyncMsg.h"
#include "SyncFrame.h"
#include "srpc.pb.h"
#include "srpc_log.h"
#include "srpc_comm.h"
#include "srpc_proto.h"
#include "srpc_channel.h"
#include "srpc_service.h"
#include "srpc_intf.h"

#include <Python.h> 
#include "py_handle.h"

#define PY_ENTRY     "entry"
#define PY_ENTRY_FILE  "./py/"PY_ENTRY".py"
#define LOG_SCREEN(lvl, fmt, args...)   do{ printf("[%d] ", getpid());printf(fmt, ##args); fflush(stdout);} while(0)

using namespace srpc;

static uint64_t g_loadfile_time;

PyObject*  g_entry_module = NULL;

// python全局析构函数
void srpc_py_handle_fini(void)
{
    if (g_entry_module == NULL)
        return;
    
    PyObject* py_func = PyObject_GetAttrString(g_entry_module, "fini"); //这里是要调用的函数名  
    if (py_func == NULL)
    {   
        NGLOG_ERROR("No function named 'fini' in entry.py");
        return;
    }

    PyObject* py_ret = PyEval_CallObject(py_func, NULL);
    if (py_ret == NULL)
    {
        NGLOG_ERROR("Call function 'fini' exception in entry.py");
    }

    Py_XDECREF(py_ret);
}

// python全局析构函数
void srpc_py_handle_loop(void)
{
    if (g_entry_module == NULL)
        return;
    
    PyObject* py_func = PyObject_GetAttrString(g_entry_module, "loop"); //这里是要调用的函数名  
    if (py_func == NULL)
    {   
        NGLOG_ERROR("No function named 'fini' in entry.py");
        return;
    }

    PyObject* py_ret = PyEval_CallObject(py_func, NULL);
    if (py_ret == NULL)
    {
        NGLOG_ERROR("Call function 'loop' exception in entry.py");
    }

    Py_XDECREF(py_ret);
}


// python全局初始化函数
int srpc_py_handler_init(const char *config)
{
    if (g_entry_module == NULL)
        return -1;
    
    PyObject* py_func = PyObject_GetAttrString(g_entry_module, "init"); //这里是要调用的函数名  
    if (py_func == NULL || !PyCallable_Check(py_func))
    {   
        NGLOG_ERROR("No function named 'init' in entry.py");
        return -1;
    }

    PyObject *py_args = PyTuple_New(1);
    PyTuple_SetItem(py_args, 0, Py_BuildValue("s", config)); 
    PyObject* py_ret = PyEval_CallObject(py_func, py_args); //调用函数,NULL表示参数为空

    int ret = -1;
    if (py_ret == NULL)
    {
        NGLOG_ERROR("No return value for init function in entry.py");
    }
    else
    {
        ret = PyInt_AsLong(py_ret);
        if (ret != 0)
        {
            NGLOG_ERROR("Init failed ret=%d in entry.py", ret);
        }
    }

    Py_XDECREF(py_ret);
    Py_DECREF(py_args);
    
    return ret;
}

// 处理函数
int srpc_py_handler_process(const string &method,  void* extInfo, int type,
                             const string &request, string &response)
{    
    if (g_entry_module == NULL)
        return -1;
    
    PyObject* py_func = PyObject_GetAttrString(g_entry_module, "process");
    if (py_func == NULL)
    {   
        NGLOG_ERROR("No function named 'process' in entry.py");
        return SRPC_ERR_PYTHON_FAILED;
    }

    PyObject *py_args = PyTuple_New(3);
    PyTuple_SetItem(py_args, 0, Py_BuildValue("s", method.c_str())); 
    PyTuple_SetItem(py_args, 1, Py_BuildValue("s#", request.c_str(), request.length()));
    PyTuple_SetItem(py_args, 2, Py_BuildValue("i", (type == 2)));
    PyObject* py_ret = PyEval_CallObject(py_func, py_args); //调用函数,NULL表示参数为空

    char*  rsp_ptr = NULL;
    int  rsp_len = 0;  
    int  ret = -1;
    if (py_ret && PyArg_ParseTuple(py_ret, "is#", &ret, &rsp_ptr, &rsp_len))
    {
        if (ret == 0)
        {
            response.assign(rsp_ptr, rsp_len);
            Py_DECREF(py_ret);
            Py_DECREF(py_args);
            return 0;
        }
        else
        {
            NGLOG_ERROR("process failed %d in entry.py, errmsg: %s", ret, response.c_str());
            Py_DECREF(py_ret);
            Py_DECREF(py_args);
            return SRPC_ERR_PYTHON_FAILED;
        }
    }
    else
    {
        NGLOG_ERROR("Illegal return value for 'process' function in entry.py");
        Py_XDECREF(py_ret);
        Py_DECREF(py_args);
        return SRPC_ERR_PYTHON_FAILED;
    }
}

int srpc_py_init(void)
{
    Py_SetPythonHome("./python");
    Py_Initialize(); 

    // 将Python工作路径切换到待调用模块所在目录
    string path = "./py";
    string chdir_cmd = string("sys.path.append(\"") + path + "\")";
    const char* cstr_cmd = chdir_cmd.c_str();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString(cstr_cmd);

    string lib_path = string("sys.path.append(\"") + "./lib" + "\")";
    PyRun_SimpleString(lib_path.c_str());

    return 0;
}

// 加载python入口文件
int srpc_py_load_file(void)
{
    const char *filename = PY_ENTRY_FILE;

    // 1. 检查文件是否有可读权限
    if (access(filename, R_OK))
    {
        LOG_SCREEN(LOG_ERROR, "Access file [%s] failed, [%m]", filename);
        return -1;
    }

    // 2. 获取文件的更新时间
    struct stat buf;
    int ret = stat(filename, &buf);
    if (ret)
    {
        LOG_SCREEN(LOG_ERROR, "Read py entry file [%s] stat failed, [%m]", filename);
        return -2;
    }

    g_loadfile_time = (uint64_t)buf.st_mtime;

    PyObject* moduleName = PyString_FromString(PY_ENTRY); //模块名，不是文件名
    PyObject* pModule = PyImport_Import(moduleName);
    if (!pModule)
    {
        LOG_SCREEN(LOG_ERROR, "Load module [%s] failed, [%m]", PY_ENTRY);
        return -3;
    }

    g_entry_module = pModule;
    return ret;
}


void srpc_py_end(void)
{
    Py_Finalize();
}


// 检查是否需要重启python引擎
int srpc_py_restart(void)
{
    // 这里只判断了入口文件是否有修改
    struct stat buf;
    int ret = stat(PY_ENTRY_FILE, &buf);
    if (0 != ret)
    {
        NGLOG_ERROR("check file [%s] stat failed, [%m]!", PY_ENTRY_FILE);
        return -1;
    }

    g_loadfile_time = (uint64_t)buf.st_mtime;

    return 0;
}

