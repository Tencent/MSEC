
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


#ifndef __SRPC_LOG_H__
#define __SRPC_LOG_H__

#include <stdint.h>
#include <map>
#include <string>

using namespace std;

namespace srpc
{

const string &SrpcServiceName(void);

/* 日志选项map */
typedef std::map<std::string, std::string> LogOptions;

/**
 * @brief 日志选项定义
 */
class CLogOption
{
public:
    CLogOption()  {}
    ~CLogOption() {}

    /**
     * @brief 设置日志选项
     */
    void Set(const char *k, const char *v);
    void Set(const char *k, int64_t v);

    /**
     * @brief 获取选项
     */
    LogOptions& Get(void);

private:
    LogOptions m_options;
};

}


#endif

