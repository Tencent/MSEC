
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
#include <map>
#include <string>
#include "config.h"
#include "srpc_cintf.h"
#include "logsys_api.h"
#include "srpc_log.h"


using namespace srpc;

typedef std::map<std::string, std::string>  LogHeaderMap;
static LogHeaderMap g_msec_log_header;

void set_log_option_str(const char *key, const char *val)
{
    g_msec_log_header[key] = val;
}

void set_log_option_long(const char *key, int64_t val)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%ld", val);
    set_log_option_str(key, buf);
}

void reset_log_option(void)
{
    g_msec_log_header.clear();
}

void msec_log_error(const char *str)
{
    LLOG_ERROR("%s", str);
    msec::LOG(msec::LogsysApi::ERROR, g_msec_log_header, str);
}

void msec_log_info(const char *str)
{
    LLOG_INFO("%s", str);
    msec::LOG(msec::LogsysApi::INFO, g_msec_log_header, str);
}

void msec_log_debug(const char *str)
{
    LLOG_DEBUG("%s", str);
    msec::LOG(msec::LogsysApi::DEBUG, g_msec_log_header, str);
}

void msec_log_fatal(const char *str)
{
    LLOG_FATAL("%s", str);
    msec::LOG(msec::LogsysApi::FATAL, g_msec_log_header, str);
}



