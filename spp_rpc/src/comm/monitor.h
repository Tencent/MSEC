
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


#ifndef __SPP_MONITOR_H__
#define __SPP_MONITOR_H__

#include <stdint.h>
#include <stdio.h>
#include <string>

using namespace std;

namespace spp
{
namespace comm
{

#define MONITOR_PROXY_DOWN              "frm.ctrl pull-up proxy"                                //spp_proxy进程down，重新拉起
#define MONITOR_WORKER_DOWN             "frm.ctrl pull-up worker"                               //spp_worker进程down，重新拉起
#define MONITOR_PROXY_MORE              "frm.ctrl kill proxy"                                   //spp_proxy进程太多，减少进程
#define MONITOR_WORKER_MORE             "frm.ctrl kill worker"                                  //spp_worker进程太多，减少进程
#define MONITOR_PROXY_DT                "frm.ctrl pull-up proxy error [D/T]"                    //spp_proxy进程处于D/T状态，无法拉起
#define MONITOR_WORKER_DT               "frm.ctrl pull-up worker error [D/T]"                   //spp_worker进程处于D/T状态，无法拉起
#define MONITOR_PROXY_LESS              "frm.ctrl start-up proxy"                               //spp_proxy进程数不足，增加进程
#define MONITOR_WORKER_LESS             "frm.ctrl start-up worker"                              //spp_worker进程数不足，增加进程
#define MONITOR_PROXY_RECV_UDP          "frm.proxy recevied UDP package"                        //proxy收到udp包 
#define MONITOR_PROXY_DROP_UDP          "frm.proxy overload, drop UDP package"                  //proxy因为过载丢弃的udp请求 
#define MONITOR_PROXY_PROC_UDP          "frm.proxy process UDP package "                        //proxy处理的udp包 
#define MONITOR_PROXY_ACCEPT_TCP        "frm.proxy recevice TCP new connection"                 //proxy收到TCP新建连接 
#define MONITOR_PROXY_REJECT_TCP        "frm.proxy overload, refuse TCP new connection"         //proxy因为过载拒绝TCP连接 
#define MONITOR_PROXY_ACCEPT_TCP_SUSS   "frm.proxy accept TCP connection"                       //proxy新建TCP连接成功 
#define MONITOR_TIMER_CLEAN_CONN        "frm.proxy close expired connection"                    //定期清理连接数量 
#define MONITOR_TIMEOUT_UDP             "frm.proxy close expired UDP vitrual connections"       //清理超时UDP连接数 
#define MONITOR_TIMEOUT_TCP             "frm.proxy close expired TCP connection"                //清理超时TCP连接数 
#define MONITOR_CONNSET_NEW_CONN        "frm.proxy add connection"                              //ConnSet新增连接数 
#define MONITOR_CONNSET_CLOSE_CONN      "frm.proxy close connection"                            //ConnSet关闭连接数 
#define MONITOR_CLOSE_UDP               "frm.proxy close UDP virtual connection"                //关闭Udp链接数 
#define MONITOR_CLOSE_TCP               "frm.proxy close TCP connection"                        //关闭TCP连接数  
#define MONITOR_PROXY_PROC_TCP          "frm.proxy process TCP request"                         //proxy处理TCP请求数 
#define MONITOR_CLIENT_CLOSE_TCP        "frm.proxy client close TCP connection"                 //客户端主动关闭TCP连接 
#define MONITOR_PROXY_PROC              "frm.proxy request number"                              //proxy处理请求数 
#define MONITOR_PROXY_TO_WORKER         "frm.proxy request number [proxy->worker]"              //proxy发送到worker的请求数 
#define MONITOR_WORKER_TO_PROXY         "frm.proxy response number [worker->proxy]"             //proxy收到worker回复请求数 
#define MONITOR_WORKER_FROM_PROXY       "frm.worker request number [proxy->worker]"             //worker收到proxy请求数 
#define MONITOR_WORKER_OVERLOAD_DROP    "frm.worker overload"                                   //worker防雪崩丢球请求数 
#define MONITOR_WORKER_PROC_SUSS        "frm.worker spp_handle_proccess success"                //worker spp_handle_process处理成功数 
#define MONITOR_WORKER_PROC_FAIL        "frm.worker spp_handle_proccess failed"                 //worker spp_handle_process处理失败数 
#define MONITOR_WORKER_RECV_DELAY_1     "frm.worker [proxy->worker] delay [0~1ms]"              //worker从队列取包延迟[0~1]ms 
#define MONITOR_WORKER_RECV_DELAY_10    "frm.worker [proxy->worker] delay (1~10ms]"             //worker从队列取包延迟(1~10]ms 
#define MONITOR_WORKER_RECV_DELAY_50    "frm.worker [proxy->worker] delay (10~50ms]"            //worker从队列取包延迟(10~50]ms 
#define MONITOR_WORKER_RECV_DELAY_100   "frm.worker [proxy->worker] delay (50~100ms]"           //worker从队列取包延迟(50~100]ms 
#define MONITOR_WORKER_RECV_DELAY_XXX   "frm.worker [proxy->worker] delay (100+ms)"             //worker从队列取包延迟100+ms 
#define MONITOR_PROXY_RELAY_DELAY_1     "frm.proxy cost [0~1ms]"                                //proxy回包延时[0~1]ms 
#define MONITOR_PROXY_RELAY_DELAY_10    "frm.proxy cost (1~10ms]"                               //proxy回包延时(1~10]ms 
#define MONITOR_PROXY_RELAY_DELAY_50    "frm.proxy cost (10~50ms]"                              //proxy回包延时(10~50]ms 
#define MONITOR_PROXY_RELAY_DELAY_100   "frm.proxy cost (50~100ms]"                             //proxy回包延时(50~100]ms 
#define MONITOR_PROXY_RELAY_DELAY_XXX   "frm.proxy cost (100+ms)"                               //proxy回包延时100+ms 
#define MONITOR_SEND_FLOWID_ERR         "frm.proxy invalid flowid for send"                     //应答消息flowid异常 
#define MONITOR_RECV_FLOWID_ERR         "frm.proxy invalid flowid for recv"                     //接收消息flowid异常  
#define MONITOR_CLOSE_FLOWID_ERR        "frm.proxy invalid flowid for close"                    //关闭连接时flowid异常 

/* monitor上报基类 */
class CMonitorBase
{
public:
    CMonitorBase(){}
    virtual ~CMonitorBase(){}

    virtual int32_t init(void *arg = NULL) = 0;
    virtual int32_t report(uint32_t id, uint32_t value = 1) = 0;	
    virtual int32_t set(uint32_t id, uint32_t value = 1) = 0;
    virtual int32_t report(const char *attr, uint32_t value = 1) = 0;
    virtual int32_t set(const char *attr, uint32_t value = 1) = 0;
};

/* Ngse上报类 */
class CNgseMonitor : public CMonitorBase
{
public:
    CNgseMonitor(const char *name): m_name(name) {}
    virtual ~CNgseMonitor() {}

    virtual inline int32_t init(void *arg = NULL)
    {
        return 0;
    }

    virtual inline int32_t report(uint32_t id, uint32_t value = 1)
    {
        return 0;
    }

    virtual inline int32_t set(uint32_t id, uint32_t value = 1)
    {
        return 0;
    }

    virtual int32_t report(const char *attr, uint32_t value = 1);
    virtual int32_t set(const char *attr, uint32_t value = 1);
    
private:
    std::string  m_name;       // 业务名
};

extern CMonitorBase *_spp_g_monitor;

/* monitor注册接口 */
static inline void MonitorRegist(CMonitorBase *monitor)
{
    _spp_g_monitor = monitor;
}

#define MONITOR(a_) do {  \
            if (_spp_g_monitor) { \
                _spp_g_monitor->report(a_);\
            }\
        } while(0)

#define MONITOR_INC(a_, v_) do { \
            if (_spp_g_monitor) { \
                _spp_g_monitor->report(a_, v_); \
            } \
        } while(0)
        
#define MONITOR_SET(a_, v_) do { \
            if (_spp_g_monitor) { \
                _spp_g_monitor->set(a_, v_); \
            } \
        } while(0)

/* RPC上报，加上"rpc."前缀 */
#define RPC_REPORT(a_) do {  \
            char attr_[128];\
            snprintf(attr_, sizeof(attr_), "frm.rpc %s", a_);\
            MONITOR(attr_);\
        } while(0)
            
#define RPC_REPORT_INC(a_, v_) do {  \
            char attr_[128];\
            snprintf(attr_, sizeof(attr_), "frm.rpc %s", a_);\
            MONITOR_INC(attr_, v_);\
        } while(0)
            
#define RPC_REPORT_SET(a_, v_) do {  \
            char attr_[128];\
            snprintf(attr_, sizeof(attr_), "frm.rpc %s", a_);\
            MONITOR_SET(attr_, v_);\
        } while(0)


/* 业务开发可使用如下宏做业务上报 */
#define ATTR_REPORT(a_) do {  \
            char attr_[128];\
            snprintf(attr_, sizeof(attr_), "usr.%s", a_);\
            MONITOR(attr_);\
        } while(0)

#define ATTR_REPORT_INC(a_, v_) do {  \
            char attr_[128];\
            snprintf(attr_, sizeof(attr_), "usr.%s", a_);\
            MONITOR_INC(attr_, v_);\
        } while(0)

#define ATTR_REPORT_SET(a_, v_) do {  \
            char attr_[128];\
            snprintf(attr_, sizeof(attr_), "usr.%s", a_);\
            MONITOR_SET(attr_, v_);\
        } while(0)

}
}

#endif

