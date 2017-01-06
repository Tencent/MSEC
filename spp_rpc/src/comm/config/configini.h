
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



#ifndef __CONFIGINI_H__
#define __CONFIGINI_H__

#include <string>

using namespace std;

#define CONF_RET_OK               0 // 获取配置成功
#define CONF_RET_FILE_NONEXIST   -1 // 配置文件不存在
#define CONF_RET_NO_CONFIG       -2 // 该配置不存在
#define CONF_RET_LOAD_ERR        -3 // 配置加载失败

/**
 * @brief 获取INI格式配置
 * @info  [USER]        -- session USER
 *        user=bison    -- key:user val:bison
 * @param config_path   [IN]   配置文件路径；不带该参数，使用系统的配置文件
 *        session       [IN]   session
 *        key           [IN]   key
 *        val           [OUT]  值
 * @return  CONF_RET_OK              成功
 *          CONF_RET_FILE_NONEXIST   配置文件不存在
 *          CONF_RET_NO_CONFIG       该配置不存在
 *          CONF_RET_LOAD_ERR        配置加载失败
 */
int GetConfig(const string &config_path, const string &session, const string &key, string &val);
int GetConfig(const string &session, const string &key, string &val);


#endif

