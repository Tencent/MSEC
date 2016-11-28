
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
#include "serverbase.h"

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

/**
 * @brief 日志适配基类
 */
class LogAdpt
{
public:

    /**
     * @brief 日志构造与析构
     */
    LogAdpt(){}
    virtual ~LogAdpt(){}

    /**
     * @brief 日志初始化及重加载接口
     */
    virtual int32_t Init() {return 0;}
    virtual int32_t Reload() {return 0;}

    /**
     * @brief 日志优先按等级过滤, 减少解析参数的开销
     * @return true 可以打印该级别, false 跳过不打印该级别
     */
    virtual bool CheckDebug(){ return false;}
    virtual bool CheckInfo() { return false;}
    virtual bool CheckError(){ return false;}
    virtual bool CheckFatal(){ return false;}

    /**
     * @brief 设置日志选项
     */
    virtual void SetOption(const char *k, const char *v) {}
    virtual void SetOption(const char *k, int64_t v) {}

    /**
     * @brief 获取日志选项
     */
    virtual CLogOption* GetOption() {return NULL;};

    /**
     * @brief 日志分级记录接口
     */    
    virtual void LogDebug(const char* fmt, ...){}
    virtual void LogInfo(const char* fmt, ...) {}
    virtual void LogError(const char* fmt, ...){}
    virtual void LogFatal(const char* fmt, ...){}
};

/**
 * @brief Ngse日志适配类
 */
class CNgLogAdpt: public LogAdpt
{
public:

    /**
     * @brief 日志构造与析构
     */
    CNgLogAdpt() {}
    virtual ~CNgLogAdpt() {}

    /**
     * @brief 日志初始化函数
     */
    int32_t Init(const char *config, const char *service_name);

    /**
     * @brief 日志重新加载函数
     */
    int32_t Reload(const char *config, const char *service_name);

    /**
     * @brief 日志优先按等级过滤, 减少解析参数的开销
     * @return true 可以打印该级别, false 跳过不打印该级别
     */
    virtual bool CheckDebug();
    virtual bool CheckInfo();
    virtual bool CheckError();
    virtual bool CheckFatal();

    /**
     * @brief 获取当前上下文的选项参数
     * @info  返回选项参数
     */
    virtual CLogOption* GetOption();

    /**
     * @brief 设置当前上下文的选项参数
     */
    virtual void SetOption(const char *k, const char *v);
    virtual void SetOption(const char *k, int64_t v);

    /**
     * @brief 日志接口
     */
    virtual void LogDebug(const char* fmt, ...);
    virtual void LogInfo(const char* fmt, ...);
    virtual void LogError(const char* fmt, ...);
    virtual void LogFatal(const char* fmt, ...);

private:
    CLogOption default_option;          // 默认选项参数
};


/**
 * @brief Spp日志适配类
 */
class CSppLogAdpt : public LogAdpt
{
public:

    /**
     * @brief 日志构造与析构
     */
    CSppLogAdpt(CServerBase* base): _base(base) {}
    virtual ~CSppLogAdpt() {}

    /**
     * @brief 日志优先按等级过滤, 减少解析参数的开销
     * @return true 可以打印该级别, false 跳过不打印该级别
     */
    virtual bool CheckDebug();
    virtual bool CheckInfo();
    virtual bool CheckError();
    virtual bool CheckFatal();

    /**
     * @brief 日志接口
     */
    virtual void LogDebug(const char* fmt, ...);
    virtual void LogInfo(const char* fmt, ...);
    virtual void LogError(const char* fmt, ...);
    virtual void LogFatal(const char* fmt, ...);

private:
    CServerBase *_base;
};


extern LogAdpt* _srpc_rlog;
extern LogAdpt* _spp_llog;

/**
 * @brief 远程日志注册接口
 */
static inline void RegisterRLog(LogAdpt* log)
{
    _srpc_rlog = log;
}

/**
 * @brief 获取注册的远程日志接口对象
 */
static inline LogAdpt* GetRlog()
{
    return _srpc_rlog;
}

/**
 * @brief 本地日志注册接口
 */
static inline void RegisterLlog(LogAdpt* log)
{
    _spp_llog = log;
}

/**
 * @brief 获取注册的本地日志接口对象
 */
static inline LogAdpt* GetLlog()
{
    return _spp_llog;
}

/* ERROR日志打印宏 */
#define LLOG_ERROR(fmt, args...) do { \
        if (_spp_llog && _spp_llog->CheckError()) { \
            _spp_llog->LogError(fmt"\n", ##args); \
        } \
    }while(0)

/* DEBUG日志打印宏 */
#define LLOG_DEBUG(fmt, args...) do { \
        if (_spp_llog && _spp_llog->CheckDebug()) { \
            _spp_llog->LogDebug(fmt"\n", ##args); \
        } \
    }while(0)

/* INFO日志打印宏 */
#define LLOG_INFO(fmt, args...) do { \
        if (_spp_llog && _spp_llog->CheckInfo()) { \
            _spp_llog->LogInfo(fmt"\n", ##args); \
        } \
    }while(0)

/* FATAL日志打印宏 */
#define LLOG_FATAL(fmt, args...) do { \
        if (_spp_llog && _spp_llog->CheckFatal()) { \
            _spp_llog->LogFatal(fmt"\n", ##args); \
        } \
    }while(0)


/* ERROR日志打印宏 */
#define RLOG_ERROR(fmt, args...) do { \
        if (_srpc_rlog && _srpc_rlog->CheckError()) { \
            _srpc_rlog->GetOption()->Set("FileLine", (int64_t)__LINE__); \
            _srpc_rlog->GetOption()->Set("Function", __FUNCTION__); \
            _srpc_rlog->LogError(fmt, ##args); \
        } \
    }while(0)

/* DEBUG日志打印宏 */
#define RLOG_DEBUG(fmt, args...) do { \
        if (_srpc_rlog && _srpc_rlog->CheckDebug()) { \
            _srpc_rlog->GetOption()->Set("FileLine", (int64_t)__LINE__); \
            _srpc_rlog->GetOption()->Set("Function", __FUNCTION__); \
            _srpc_rlog->LogDebug(fmt, ##args); \
        } \
    }while(0)

/* INFO日志打印宏 */
#define RLOG_INFO(fmt, args...) do { \
        if (_srpc_rlog && _srpc_rlog->CheckInfo()) { \
            _srpc_rlog->GetOption()->Set("FileLine", (int64_t)__LINE__); \
            _srpc_rlog->GetOption()->Set("Function", __FUNCTION__); \
            _srpc_rlog->LogInfo(fmt, ##args); \
        } \
    }while(0)

/* FATAL日志打印宏 */
#define RLOG_FATAL(fmt, args...) do { \
        if (_srpc_rlog && _srpc_rlog->CheckFatal()) { \
            _srpc_rlog->GetOption()->Set("FileLine", (int64_t)__LINE__); \
            _srpc_rlog->GetOption()->Set("Function", __FUNCTION__); \
            _srpc_rlog->LogFatal(fmt, ##args); \
        } \
    }while(0)

/* 选项设置宏 */
#define RLOG_SET_OPTION(k, v) do { \
        if (_srpc_rlog){ \
            _srpc_rlog->SetOption(k, v); \
        } \
    }while (0)

/* NGSE ERROR日志 */
#define NGLOG_ERROR(fmt, args...)   do { \
        LLOG_ERROR(fmt, ##args);\
        RLOG_ERROR(fmt, ##args);\
    } while(0)

/* NGSE DEBUG日志 */
#define NGLOG_DEBUG(fmt, args...)   do { \
        LLOG_DEBUG(fmt, ##args);\
        RLOG_DEBUG(fmt, ##args);\
    } while (0)

/* NGSE INFO日志 */
#define NGLOG_INFO(fmt, args...)   do { \
        LLOG_INFO(fmt, ##args);\
        RLOG_INFO(fmt, ##args);\
    } while (0)

/* NGSE FATAL日志 */
#define NGLOG_FATAL(fmt,args...)  do { \
        LLOG_FATAL(fmt, ##args); \
        RLOG_FATAL(fmt, ##args); \
    }while(0)

/* NGSE 选项设置宏 */
#define NGLOG_SET_OPTION(k, v)  RLOG_SET_OPTION(k, v)

}

#endif

