
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


/**
 *  @file SyncFrame.cpp
 *  @info 同步线程状态框架实现
 *  @time 20130515
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>

#include "tlog.h"
#include "tcommu.h"
#include "serverbase.h"
#include "comm_def.h"
#include "frame.h"
#include "defaultworker.h"
#include "IFrameStat.h"
#include "StatComDef.h"

#include "mt_msg.h"
#include "mt_version.h"
#include "micro_thread.h"

#include "SyncMsg.h"
#include "SyncFrame.h"
#include "monitor.h"

using namespace std;
using namespace spp::worker;
using namespace spp::statdef;
using namespace NS_MICRO_THREAD;
using namespace SPP_SYNCFRAME;

#define MONITOR_SYNC_START          "syncframe start"                   // 启动
#define MONITOR_SYNC_STOP           "syncframe stop"                    // 完毕
#define MONITOR_SYNC_ILL_MSG        "syncframe illegal msg"             // 消息非法
#define MONITOR_SYNC_MSG_FAIL       "syncframe msg failed"              // 消息失败
#define MONITOR_SYNC_CREATE_FAIL    "syncframe create microthread fail" // 创建失败
#define MONITOR_SYNC_MSG_SIZE       "syncframe msg count"               // 微线程消息数

namespace SPP_SYNCFRAME {
/**
 * @brief SPP日志适配微线程框架定义部分
 */
#define LOG_MAX_LEN     4096
class SppLogAdpt : public LogAdapter
{
private:
    CServerBase *_base;                    ///< spp的服务器对象
    char        _buff[LOG_MAX_LEN];        ///< 日志临时缓存
    
public:

    /**
     * @brief 构造与析构
     */
    SppLogAdpt(CServerBase* base)  :  _base(base) {
    }; 
    virtual ~SppLogAdpt(){};

    /**
     * @brief 日志级别过滤检查接口实现
     */
    bool CheckLogLvl(LOG_LEVEL lvl) {
        if (!_base) { 
            return false;
        }
        
        int iLogLvl = _base->log_.log_level(-1); 
        if (lvl >= iLogLvl) {
            return true;
        } else {
            return false;
        }
    }

    virtual bool CheckTrace() {
        return CheckLogLvl(LOG_TRACE); //  lvl 1
    };    
    virtual bool CheckDebug() {
        return CheckLogLvl(LOG_DEBUG);  //  lvl 2
    };
    virtual bool CheckError() { 
        return CheckLogLvl(LOG_ERROR);  //  lvl 3
    };
    

    /**
     * @brief DEBUG接口实现
     */
    virtual void LogDebug(char* fmt, ...){
        if (_base) {            
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(_buff, LOG_MAX_LEN - 1, fmt, ap);
            va_end(ap);          
            _base->log_.log_i(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, \
               LOG_DEBUG, "%s\n", _buff);
        }
    };

    /**
     * @brief TRACE接口实现
     */
    virtual void LogTrace(char* fmt, ...){
        if (_base) {        
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(_buff, LOG_MAX_LEN - 1, fmt, ap);
            va_end(ap);          
            _base->log_.log_i(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, \
               LOG_INFO, "%s\n", _buff);
        }
    };

    /**
     * @brief ERROR接口实现
     */
    virtual void LogError(char* fmt, ...){           
        if (_base) { 
            va_list ap;
            va_start(ap, fmt);
            vsnprintf(_buff, LOG_MAX_LEN - 1, fmt, ap);
            va_end(ap);  
            _base->log_.log_i(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, \
               LOG_ERROR, "%s\n", _buff);
        }
    }; 

    /**
     * @brief attr接口实现
     */
    virtual void AttrReportAdd(int attr, int iValue) {
    	MONITOR_INC(attr, iValue);
    };


	/**
     * @brief attr接口实现
     */
    virtual void AttrReportSet(int attr, int iValue) {
    	MONITOR_SET(attr, iValue);
		
		if (_base) {
			CDefaultWorker* worker = (CDefaultWorker*)_base;
			switch(attr)
			{			
				case 492069://微线程池大小						
					worker->fstat_.op(WIDX_MT_THREAD_NUM, iValue);
					break;
				case 493212://微线程消息数				
					worker->fstat_.op(WIDX_MT_MSG_NUM, iValue);
					break;
			}
    	}
    };

    /**
     * @brief attr接口实现
     */
    virtual void AttrReportAdd(const char *attr, int iValue) {
        MONITOR_INC(attr, iValue);
    };


    /**
     * @brief attr接口实现
     */
    virtual void AttrReportSet(const char *attr, int iValue) {
        MONITOR_SET(attr, iValue);
    };
};




/**
 * 线程统一入口函数, 会调用消息的处理函数
 *
 * @param pMsg CMsgBase派生类对象指针，存放和请求相关的数据，该对象需要插件以new的方式分配，释放由框架负责
 *
 * @return 0: 成功； 其他：失败
 *
 */ 
void ThreadEntryFunc(void* arg)
{
	CSyncFrame::Instance()->_uMsgNo++;
    MT_ATTR_API_SET(MONITOR_SYNC_MSG_SIZE, CSyncFrame::Instance()->_uMsgNo); // 微线程消息数

    MT_ATTR_API(MONITOR_SYNC_START, 1); // 启动
    
    int rc = 0;
    CSyncMsg* msg = (CSyncMsg*)arg;
    if (!msg)
    {
        MT_ATTR_API(MONITOR_SYNC_ILL_MSG, 1); // 消息非法
        SF_LOG(LOG_ERROR, "Invalid arg, error");
        goto EXIT_LABEL;
    }

    rc = msg->HandleProcess();
    if (rc != 0)
    { 
        MT_ATTR_API(MONITOR_SYNC_MSG_FAIL, 1); // 消息失败
        SF_LOG(LOG_DEBUG, "User msg process failed(%d), close connset", rc);
        
        blob_type rspblob;        
        rspblob.len = 0;
        rspblob.data = NULL;
        msg->SendToClient(rspblob);
    }

EXIT_LABEL:

    if (arg) {		
		CSyncFrame::Instance()->_uMsgNo--;
		STEP_REQ_STAT(msg->GetMsgCost());
        delete msg;
    } 

    MT_ATTR_API(MONITOR_SYNC_STOP, 1); // 完毕

    return;
};


/**
 *  摘自SPP的notify处理, 省去文件依赖
 */
int SppShmNotify(int key)
{
    if(key <= 0) {
        return -1;
    }
    
    int netfd = -1;
    char path[32] = {0};
    snprintf(path, sizeof(path), ".notify_%d", key);

    if(mkfifo(path, 0666) < 0) {
        if (errno != EEXIST) {
            printf("create fifo[%s] error[%m]\n", path);
            return -2;
        } 
    }

    netfd = open(path, O_RDWR | O_NONBLOCK, 0666);
    if(netfd == -1) {
        printf("open notify fifo[%s] error[%m]\n", path);
        return -3;
    }

    return netfd;
}


/**
 *  @brief  同步框架插件全局成员初始化
 */
CSyncFrame *CSyncFrame::_s_instance = NULL;
bool CSyncFrame::_init_flag  = false;
int  CSyncFrame::_iNtfySock  = -1;


/**
 *  @brief  同步框架插件全局句柄获取
 */
CSyncFrame* CSyncFrame::Instance (void)
{
    if( NULL == _s_instance )
    {
        _s_instance = new CSyncFrame;
    }

    return _s_instance;
}


/**
 *  @brief  同步框架插件构造
 */
CSyncFrame::CSyncFrame() 
{
    _pServBase = NULL;
    _iGroupId  = 0;
    _iNtfyFd   = -1;
	_uMsgNo    = 0;
}

/**
 *  @brief  同步框架插件析构
 */
CSyncFrame::~CSyncFrame() 
{ 
    if (_iNtfyFd != -1)
    {
        close(_iNtfyFd);
        _iNtfyFd = -1;
    }
}

/**
 *  @brief  同步框架插件反初始化
 */
void CSyncFrame::Destroy (void)
{
    if (!CSyncFrame::_init_flag)
    {
        return;
    }
    
    MtFrame::Instance()->Destroy();
    
    if( _s_instance != NULL )
    {
        delete _s_instance;
        _s_instance = NULL;
    }

    CSyncFrame::_init_flag = false;

    return;
}

/**
 *  @brief  同步框架插件初始化
 */
int CSyncFrame::InitFrame(CServerBase *pServBase, int max_thread_num, int min_thread_num)
{
    if (CSyncFrame::_init_flag)
    {
        SF_LOG(LOG_ERROR, "CSyncFrame already init ok");
        return 0;
    }
    CSyncFrame::_init_flag = true;  // 同步线程标记位
    
    _pServBase = pServBase;    

    ///< 初始化MT库
    static SppLogAdpt s_log(_pServBase);
    MtFrame::Instance()->SetDefaultThreadNum(min_thread_num);
    bool rc = MtFrame::Instance()->InitFrame(&s_log, max_thread_num);
    if (!rc)
    {
        SF_LOG(LOG_ERROR, "InitFrame sync failed; rc %d", rc);
        return -2;
    }

    ///< 取GROUPID, 侦听命名管道, 获取可读事件通知 20130523
    _iNtfyFd = SppShmNotify(_iGroupId*2); // notify comsume
    int flags;
    if ((flags = fcntl(_iNtfyFd, F_GETFL, 0)) < 0 || 
          fcntl(_iNtfyFd, F_SETFL, flags | O_NONBLOCK) < 0) 
    {
        close(_iNtfyFd);
        _iNtfyFd = -1;
        SF_LOG(LOG_ERROR, "set non block failed; rc %d", rc);
        return -2;
    }
    
    MtFrame::sleep(0);
    return 0;
}


/**
 * 同步消息的调度入口
 *
 * @param pMsg CMsgBase派生类对象指针，存放和请求相关的数据，该对象需要插件以new的方式分配，释放由框架负责
 *
 * @return 0: 成功； 其他：失败
 *
 */ 
int CSyncFrame::Process(CSyncMsg *pMsg)
{
    if (MtFrame::CreateThread(ThreadEntryFunc, pMsg) == NULL) 
    {
        MT_ATTR_API(MONITOR_SYNC_CREATE_FAIL, 1); // 创建失败
        SF_LOG(LOG_ERROR, "Sync frame start thread failed, error");
        delete pMsg;
        return -1;
    }
    return 0;
};

/**
 *  主线程循环, 每次切换上下文, 让出CPU, 调度框架后端epoll
 */
void CSyncFrame::WaitNotifyFd(int utime)
{
    static char szbuf[1024];
    if (CSyncFrame::_iNtfySock != -1)
    {
        MtFrame::WaitEvents(CSyncFrame::_iNtfySock, EPOLLIN, utime);
    }
    else
    {
        MtFrame::read(_iNtfyFd, szbuf, 1024, utime);
    }
}


/**
 *  微线程启动标志, SPP注册内部接口
 */
bool CSyncFrame::GetMtFlag()
{
    return CSyncFrame::_init_flag;
}

/**
 *  主线程循环, 根据前端是否有请求, 决定epoll等待机制
 */
void CSyncFrame::HandleSwitch(bool block)
{
    static int loopcnt = 0;

    if (block)
    {
        CSyncFrame::Instance()->WaitNotifyFd(10);
    }
    else
    {
        loopcnt++;
        int run_cnt = MtFrame::Instance()->RunWaitNum();
        if ((run_cnt >= 10) || (loopcnt % 100 == 0))
        {
            MtFrame::sleep(0);
        }    
    }
}

CSyncMsg* CSyncFrame::GetCurrentMsg()
{
    CSyncMsg *msg = NULL;
    MicroThread *thread = MtFrame::Instance()->GetRootThread();
    if (thread)
    {
        msg = (CSyncMsg *)thread->GetThreadArgs();
    }

    return msg;
}

bool CSyncFrame::CheckCurrentStack(long lESP)
{
	char *esp = (char *)lESP;
	
	MicroThread *thread = MtFrame::Instance()->GetActiveThread();
    if (thread)
    {
		return  thread->CheckStackHealth(esp);
    }

	return true;
}

int CSyncFrame::GetThreadNum()
{
	return MtFrame::Instance()->GetUsedNum();
}

void CSyncFrame::SetGroupId(int id)
{
    _iGroupId = id;
}

void CSyncFrame::sleep(int ms)
{
	MtFrame::Instance()->sleep(ms);
}

}
