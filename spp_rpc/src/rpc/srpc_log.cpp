
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
#include "SyncMsg.h"
#include "SyncFrame.h"
#include "srpc_log.h"
#include "srpc_service.h"
#include "logsys_api.h"
#include "tlog.h"

using namespace std;
using namespace srpc;

namespace srpc
{

LogAdpt* _srpc_rlog = NULL;
LogAdpt* _spp_llog = NULL;
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

/**
 * @brief 日志初始化函数
 */
int32_t CNgLogAdpt::Init(const char *config, const char *service_name)
{
    int32_t ret;
    msec::LogsysApi *api = msec::LogsysApi::GetInstance();

    _srpc_service_name = service_name;

    ret = api->Init(config);
    if (ret)
    {
        return ret;
    }

    // 设置默认选项参数，框架打印日志使用
    default_option.Set("ReqID", "0");
    default_option.Set("ClientIP", "0");
    default_option.Set("ServerIP", "0");
    default_option.Set("RPCName", "0");
    default_option.Set("Caller", "0");
    default_option.Set("Coloring", "0");
    default_option.Set("ServiceName", service_name);

    return 0;
}

/**
 * @brief 日志重新加载函数
 */
int32_t CNgLogAdpt::Reload(const char *config, const char *service_name)
{
    return Init(config, service_name);
}

/**
 * @brief 日志优先按等级过滤, 减少解析参数的开销
 * @return true 可以打印该级别, false 跳过不打印该级别
 */
bool CNgLogAdpt::CheckDebug()
{
    CLogOption* option = GetOption();
    if (msec::LogIsWritable(msec::LogsysApi::DEBUG, option->Get()))
    {
        return true;
    }

    return false;
}

/**
 * @brief 日志优先按等级过滤, 减少解析参数的开销
 * @return true 可以打印该级别, false 跳过不打印该级别
 */
bool CNgLogAdpt::CheckInfo()
{
    CLogOption* option = GetOption();
    if (msec::LogIsWritable(msec::LogsysApi::INFO, option->Get()))
    {
        return true;
    }

    return false;
}


/**
 * @brief 日志优先按等级过滤, 减少解析参数的开销
 * @return true 可以打印该级别, false 跳过不打印该级别
 */
bool CNgLogAdpt::CheckError()
{
    CLogOption* option = GetOption();
    if (msec::LogIsWritable(msec::LogsysApi::ERROR, option->Get()))
    {
        return true;
    }

    return false;
}


/**
 * @brief 日志优先按等级过滤, 减少解析参数的开销
 * @return true 可以打印该级别, false 跳过不打印该级别
 */
bool CNgLogAdpt::CheckFatal()
{
    CLogOption* option = GetOption();
    if (msec::LogIsWritable(msec::LogsysApi::FATAL, option->Get()))
    {
        return true;
    }

    return false;
}


/**
 * @brief 获取当前上下文的选项参数
 * @info  返回选项参数
 */
CLogOption* CNgLogAdpt::GetOption()
{
    CRpcMsgBase *msg = (CRpcMsgBase *)CSyncFrame::Instance()->GetCurrentMsg();
    if (msg)
    {
        return &msg->GetLogOption();
    }

    return &default_option;
}

/**
 * @brief 设置当前上下文的选项参数
 */
void CNgLogAdpt::SetOption(const char *k, const char *v)
{
    GetOption()->Set(k, v);
}

/**
 * @brief 设置当前上下文的选项参数
 */
void CNgLogAdpt::SetOption(const char *k, int64_t v)
{
    GetOption()->Set(k, v);
}

/**
 * @brief DEBUG日志接口(带选项)
 */
void CNgLogAdpt::LogDebug(const char* fmt, ...)
{
    CLogOption* option = GetOption();
    va_list ap;
    va_start(ap, fmt);
    msec::LOG((int)msec::LogsysApi::DEBUG, option->Get(), fmt, ap);
    va_end(ap);          
}

/**
 * @brief INFO日志接口(带选项)
 */
void CNgLogAdpt::LogInfo(const char* fmt, ...)
{
    CLogOption* option = GetOption();
    va_list ap;
    va_start(ap, fmt);
    msec::LOG((int)msec::LogsysApi::INFO, option->Get(), fmt, ap);
    va_end(ap);          
}

/**
 * @brief ERROR日志接口(带选项)
 */
void CNgLogAdpt::LogError(const char* fmt, ...)
{
    CLogOption* option = GetOption();
    va_list ap;
    va_start(ap, fmt);
    msec::LOG((int)msec::LogsysApi::ERROR, option->Get(), fmt, ap);
    va_end(ap);          
}

/**
 * @brief FATAL日志接口(带选项)
 */
void CNgLogAdpt::LogFatal(const char* fmt, ...)
{
    CLogOption* option = GetOption();
    va_list ap;
    va_start(ap, fmt);
    msec::LOG((int)msec::LogsysApi::FATAL, option->Get(), fmt, ap);
    va_end(ap);          
}

/**
 * @brief 日志优先按等级过滤, 减少解析参数的开销
 * @return true 可以打印该级别, false 跳过不打印该级别
 */
bool CSppLogAdpt::CheckDebug()
{
    int iLogLvl = _base->log_.log_level(-1); 
    if (tbase::tlog::LOG_DEBUG >= iLogLvl) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief 日志优先按等级过滤, 减少解析参数的开销
 * @return true 可以打印该级别, false 跳过不打印该级别
 */
bool CSppLogAdpt::CheckError()
{
    int iLogLvl = _base->log_.log_level(-1); 
    if (tbase::tlog::LOG_ERROR >= iLogLvl) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief 日志优先按等级过滤, 减少解析参数的开销
 * @return true 可以打印该级别, false 跳过不打印该级别
 */
bool CSppLogAdpt::CheckInfo()
{
    int iLogLvl = _base->log_.log_level(-1); 
    if (tbase::tlog::LOG_INFO >= iLogLvl) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief 日志优先按等级过滤, 减少解析参数的开销
 * @return true 可以打印该级别, false 跳过不打印该级别
 */
bool CSppLogAdpt::CheckFatal()
{
    int iLogLvl = _base->log_.log_level(-1); 
    if (tbase::tlog::LOG_FATAL >= iLogLvl) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief DEBUG日志接口
 */
void CSppLogAdpt::LogDebug(const char* fmt, ...)
{
    if (_base) {            
        va_list ap;
        va_start(ap, fmt);
        _base->log_.log_i_va(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, tbase::tlog::LOG_DEBUG, fmt, ap);
        va_end(ap);          
    }
}

/**
 * @brief ERROR日志接口
 */
void CSppLogAdpt::LogError(const char* fmt, ...)
{
    if (_base) {            
        va_list ap;
        va_start(ap, fmt);
        _base->log_.log_i_va(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, tbase::tlog::LOG_ERROR, fmt, ap);
        va_end(ap);          
    }
}

/**
 * @brief INFO日志接口
 */
void CSppLogAdpt::LogInfo(const char* fmt, ...)
{
    if (_base) {            
        va_list ap;
        va_start(ap, fmt);
        _base->log_.log_i_va(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, tbase::tlog::LOG_INFO, fmt, ap);
        va_end(ap);          
    }
}

/**
 * @brief FATAL日志接口
 */
void CSppLogAdpt::LogFatal(const char* fmt, ...)
{
    if (_base) {            
        va_list ap;
        va_start(ap, fmt);
        _base->log_.log_i_va(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, tbase::tlog::LOG_FATAL, fmt, ap);
        va_end(ap);          
    }
}

}


