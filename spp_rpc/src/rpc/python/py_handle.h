
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


#ifndef _HANDLER_PHP_H_
#define _HANDLER_PHP_H_

#include <string>

using namespace std;

// python全局析构函数
void srpc_py_handle_fini(void);

// python全局初始化函数
int srpc_py_handler_init(const char *config);

// python loop函数
void srpc_py_handle_loop(void);

// 处理函数
int srpc_py_handler_process(const string &method,  void* extInfo, int type, const string &request, string &response);

// 初始化python引擎, 开发者可以自己上传php.ini
int srpc_py_init(void);

// 加载python入口文件
int srpc_py_load_file(void);

// 终止python引擎
void srpc_py_end(void);

// 检查是否需要重启python引擎
int srpc_py_restart(void);

#endif 

