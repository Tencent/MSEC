
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

// PHP全局析构函数
void srpc_php_handle_fini(void);

// PHP loop函数
void srpc_php_handler_loop(void);

// PHP全局初始化函数
int srpc_php_handler_init(const char *config);

// 处理函数
int srpc_php_handler_process(const string &method,  void* extInfo, int type, const string &request, string &response);

// 初始化PHP引擎, 开发者可以自己上传php.ini
int srpc_php_init(void);

// 加载php入口文件
int srpc_php_load_file(void);

// 加载php5动态库
int srpc_php_dl_php_lib(void);

// 终止PHP引擎
void srpc_php_end(void);

// 检查内存健康度
bool srpc_check_memory_health(float water);

#endif 

