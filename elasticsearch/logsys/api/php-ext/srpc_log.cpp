
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


#include <stdint.h>
#include <string>
#include <stdio.h>
#include "srpc_log.h"
#include "logsys_api.h"

using namespace std;
using namespace srpc;

namespace srpc
{

static string _srpc_service_name;

const string &SrpcServiceName(void)
{
    return _srpc_service_name;
}

/**
 * @brief 设置日志选项
 */
void CLogOption::Set(const char *k, const char *v)
{
    m_options[k] = v;
}

/**
 * @brief 设置日志选项
 */
void CLogOption::Set(const char *k, int64_t v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)v);
    m_options[k] = buf;
}

/**
 * @brief 获取选项
 */
LogOptions& CLogOption::Get(void)
{
    return m_options;
}

}
