
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


#include "monitor.h"
#include "monitor_client.h"

namespace spp
{
namespace comm
{

CMonitorBase *_spp_g_monitor = NULL;

int32_t CNgseMonitor::report(const char *attr, uint32_t value)
{
    if (NULL == attr || attr[0] == '\0')
    {
        return -1;
    }

    return Monitor_Add(m_name.c_str(), attr, value);
}

int32_t CNgseMonitor::set(const char *attr, uint32_t value)
{
    if (NULL == attr || attr[0] == '\0')
    {
        return -1;
    }

    return Monitor_Set(m_name.c_str(), attr, value);
}

}
}

