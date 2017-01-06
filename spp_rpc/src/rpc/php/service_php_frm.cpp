
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
 *  @filename service_demo.cpp
 *  @info     demo模板
 */
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include "spp_version.h"       //框架版本号
#include "tlog.h"                      //日志
#include "tstat.h"                     //统计
#include "tcommu.h"            //通讯组件
#include "serverbase.h"        //服务器容器
#include "monitor.h"
#include "mt_incl.h"
#include "SyncMsg.h"        
#include "SyncFrame.h"
#include "srpc.pb.h"
#include "srpc_log.h"
#include "srpc_comm.h"
#include "srpc_proto.h"
#include "srpc_channel.h"
#include "srpc_service.h"
#include "srpc_intf.h"
#include "srpc_cintf.h"
#include "php_handle.h"
#include "srpc_proto_php.h"
#include "http_support.h"

using namespace tbase::tlog;
using namespace tbase::tstat;
using namespace tbase::tcommu;
using namespace spp::comm;
using namespace spp::comm;
using namespace NS_MICRO_THREAD;
using namespace SPP_SYNCFRAME;
using namespace srpc;

static char *g_original_config;

#define PHP_STDOUT_STDERR_FILE  "../log/stdout_stderr.log"

extern "C" void spp_handle_fini(void* arg1, void* arg2);

int redirect_output(void)
{
    int fd = open(PHP_STDOUT_STDERR_FILE, O_CREAT | O_WRONLY | O_APPEND, 0666);
    if (fd == -1) {
        NGLOG_ERROR("open %s failed, [%m].", PHP_STDOUT_STDERR_FILE);
        return -1;
    }

    dup2(fd, 1);
    dup2(fd, 2);
    return 0;
}

void check_output(void)
{
    static time_t last_check_time = time(NULL);
    time_t now;
    off_t  off;

    now = time(NULL);
    if (now != last_check_time)
    {
        off = lseek(1, 0, SEEK_END);
        if (off == (off_t)-1) {
            return;
        }

        if (off > 10*1024*1024) {
            truncate(PHP_STDOUT_STDERR_FILE, 0);
        }

        last_check_time = now;
    }
}


int srpc_init(const char *conf)
{
    // 载入动态库
    if (srpc_php_dl_php_lib())
    {
        NGLOG_ERROR("Load libphp5.so failed.");
        return -1;
    }
    
    // 初始化PHP的运行环境
    if (srpc_php_init())
    {
        NGLOG_ERROR("Php init failed.");
        return -2;
    }
    
    // 载入公共php文件,入口文件
    if (srpc_php_load_file())
    {
        NGLOG_ERROR("Load entry.php failed.");
        return -3;
    }
    
    // 调用php初始化函数
    if (srpc_php_handler_init(conf))
    {
        NGLOG_ERROR("Php userspace init failed.");
        return -4;
    }

    return 0;
}

int srpc_fini(void)
{
    // 调用析构函数
    srpc_php_handle_fini();
    
    // 退出PHP执行环境
    srpc_php_end();

    return 0;
}

int srpc_restart()
{
    srpc_fini();
    srpc_init(g_original_config);

    return 0;
}

void php_env_check(void)
{
    if (!srpc_check_memory_health(0.8))
    {
        NGLOG_ERROR("Php memory overlimit. restart ...");
        srpc_restart();
    }
}

/**
 * @brief 业务模块初始化插件接口(proxy,worker)
 * @param conf -业务配置文件信息
 * @param server -业务进程信息
 * @return 0 - 成功, 其它失败
 */
extern "C" int spp_handle_init(void* arg1, void* arg2)
{
    int ret;
    CServerBase* base   = (CServerBase*)arg2;

    if (base->servertype() == SERVER_TYPE_WORKER) // WORKER进程初始化
    {
        g_original_config   = (char*)arg1;

        redirect_output();

        ret = srpc_init(g_original_config);
        if (ret < 0) {
            return ret;
        }

        srpc_php_set_service_name(base->servicename().c_str(), (int)(base->servicename().size() + 1));
    }

    NGLOG_DEBUG("spp_handle_init php success.");

    return 0;
}


/**
 * @brief 业务模块检查报文合法性与分包接口(proxy)
 * @param block -消息指针块信息
 * @param server -业务进程信息
 * @return ==0  数据包还未完整接收,继续等待
 *         > 0  数据包已经接收完整, 返回包长度
 *         < 0  数据包非法, 连接异常, 将断开TCP连接
 */
extern "C" int spp_handle_input(unsigned flow, void* arg1, void* arg2)
{
    int32_t ret;
    blob_type* blob = (blob_type*)arg1;
    TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;

    NGLOG_DEBUG("spp_handle_input flow:%d, buffer len:%d, client ip:%s",
                 flow,
                 blob->len,
                 inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));

    if (CHttpHelper::IsSupportHttpRequest((char *)blob->data, blob->len))
    {
        CHttpHelper http_helper(HTTP_HELPER_TYPE_PROXY);
        ret = http_helper.Parse((char *)blob->data, blob->len);
    }
    else
    {
        ret = SrpcCheckPkgLen((void*)blob->data, blob->len);
    }

    // 如果UDP包，一次包没有收完，表示错误
    if ((extinfo->type_ == SOCK_TYPE_UDP) && (ret == 0))
    {
        NGLOG_ERROR("spp_handle_input udp package failed: flow:%d, buffer len:%d, client ip:%s",
                    flow,
                    blob->len,
                    inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));
        return SRPC_ERR_INVALID_PKG;
    }

    // 如果解包失败，打印错误日志
    if (ret < 0)
    {
        NGLOG_ERROR("spp_handle_input failed: flow:%d, buffer len:%d, client ip:%s, error msg: %s",
                    flow,
                    blob->len,
                    inet_ntoa(*(struct in_addr*)&extinfo->remoteip_),
                    errmsg(ret));
    }
   
    return ret; 
}


/**
 * @brief 业务模块报文按worker组分发接口(proxy)
 * @param block -消息指针块信息
 * @param server -业务进程信息
 * @return 处理该报文的组id信息
 */
extern "C" int spp_handle_route(unsigned flow, void* arg1, void* arg2)
{
    NGLOG_DEBUG("spp_handle_route, flow:%d\n", flow);
    
    // MSEC只有一个worker组，固定路由到组1
    return 1;
}


extern "C" int process_http(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob         = (blob_type*)arg1;
    CTCommu* commu          = (CTCommu*)blob->owner;
    TConnExtInfo* extinfo   = (TConnExtInfo*)blob->extdata;
    CServerBase* base       = (CServerBase*)arg2;
    char attr[256];
    CHttpHelper http_helper(HTTP_HELPER_TYPE_WORKER);
    std::string method_name;
    std::string body;
    std::string rsp_body;
    char *pkg = NULL;
    int pkg_len = 0;
    int err = SRPC_SUCCESS;
    CRpcHead  rpc_head;

    // 1. 解析http+json报文
    int ret = http_helper.Parse(blob->data, blob->len);
    if (ret < 0)
    {
        err = SRPC_ERR_INVALID_PKG;
        NGLOG_ERROR("%s", errmsg(err));
        goto EXIT_LABEL;
    }

    // 2. 获取方法名
    method_name = http_helper.GetMethodName();
    if (method_name.size() < 5)
    {
        err = SRPC_ERR_INVALID_PKG;
        NGLOG_ERROR("%s", errmsg(err));
        goto EXIT_LABEL;
    }

    // 3. 获取包体
    body = http_helper.GetBody();
    if (body.size() == 0)
    {
        err = SRPC_ERR_INVALID_PKG;
        NGLOG_ERROR("%s", errmsg(err));
        goto EXIT_LABEL;
    }

    // 4. 序列化报文
    rpc_head.set_method_name(method_name);
    rpc_head.set_sequence(newseq());
    rpc_head.set_flow_id(rpc_head.sequence());

    // 5. 上报monitor收到调用方请求，填写日志选项
    snprintf(attr, sizeof(attr), "caller [%s]", http_helper.GetCaller().c_str());
    RPC_REPORT(attr);
    set_log_option_long("ReqID", (int64_t)rpc_head.flow_id());
    set_log_option_str("ClientIP", inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));
    set_log_option_str("ServerIP", inet_ntoa(*(struct in_addr*)&extinfo->localip_));
    set_log_option_str("RPCName", rpc_head.method_name().c_str());
    set_log_option_str("Caller", "http.json");
    set_log_option_str("ServiceName", base->servicename().c_str());
    set_log_option_str("Coloring", "1");
    
    msec_log_info("srpc system water");
    set_log_option_long("Coloring", (int64_t)rpc_head.coloring());

    // 6. 调用处理函数
    srpc_php_set_header(&rpc_head);
    ret = srpc_php_handler_process(rpc_head.method_name(), extinfo, 2, body, rsp_body);
    if (ret != SRPC_SUCCESS)
    {
        RPC_REPORT(errmsg(ret));
        msec_log_error(errmsg(ret));
        err = ret;
        goto EXIT_LABEL;
    }

    pkg = (char *)rsp_body.c_str();
    pkg_len = (int)rsp_body.size();

EXIT_LABEL:
    blob_type resblob;
    string response;

    CHttpHelper::GenJsonResponse(err, pkg, pkg_len, response);
    resblob.data = (char *)response.data();
    resblob.len  = (int)response.size();
    commu->sendto(flow, &resblob, arg2);
    php_env_check();

    return 0;  // 只要成功回包, 都返回0
}

extern "C" void spp_handle_loop(void *arg)
{
    srpc_php_handler_loop();
}

/**
 * @brief 业务模块报文, worker组的处理接口(worker)
 * @param block -消息指针块信息
 * @param server -业务进程信息
 * @return 0 处理成功, 其它为失败
 */
extern "C" int spp_handle_process(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob         = (blob_type*)arg1;
    CTCommu* commu          = (CTCommu*)blob->owner;
    TConnExtInfo* extinfo   = (TConnExtInfo*)blob->extdata;
    CServerBase* base       = (CServerBase*)arg2;
    blob_type resblob;
    string req_body;
    string rsp_body;
    int32_t msg_len;
    char*   rsp_buff;
    char attr[256];

    NGLOG_DEBUG("spp_handle_process flow:%d, buffer len:%d, client ip:%s",
                flow,
                blob->len,
                inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));

    check_output();

    if (CHttpHelper::IsSupportHttpRequest((char *)blob->data, blob->len))
    {
        return process_http(flow, arg1, arg2);
    }    

    // 1. 解析消息头信息
    CRpcHead  rpc_head;
    int32_t ret = SrpcUnpackPkgHead((char*)blob->data, blob->len, &rpc_head);
    if (ret != SRPC_SUCCESS)
    {
        RPC_REPORT(errmsg(ret));
        rpc_head.set_err(ret);
        NGLOG_ERROR("%s", errmsg(ret));
        goto EXIT_LABEL;
    }

    NGLOG_DEBUG("parse msg head: %s", rpc_head.DebugString().c_str());

    // 2. 上报monitor收到调用方请求，填写日志选项
    snprintf(attr, sizeof(attr), "caller [%s]", rpc_head.caller().c_str());    
    RPC_REPORT(attr);

    
    if (!rpc_head.has_flow_id() || !rpc_head.flow_id())
    {
        rpc_head.set_flow_id(newseq());
    }
    set_log_option_long("ReqID", (int64_t)rpc_head.flow_id());
    set_log_option_str("ClientIP", inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));
    set_log_option_str("ServerIP", inet_ntoa(*(struct in_addr*)&extinfo->localip_));
    set_log_option_str("RPCName", rpc_head.method_name().c_str());
    set_log_option_str("Caller", rpc_head.caller().c_str());
    set_log_option_str("ServiceName", base->servicename().c_str());
    //set_log_option_str("Coloring", "1");
    
    //msec_log_info("srpc system water");
    set_log_option_long("Coloring", (int64_t)rpc_head.coloring());


    // 3. 获取包体
    ret = SrpcGetPkgBodyString((char*)blob->data, blob->len, req_body);
    if (ret != SRPC_SUCCESS)
    {
        RPC_REPORT(errmsg(ret));
        rpc_head.set_err(ret);
        msec_log_error(errmsg(ret));
        goto EXIT_LABEL;
    }

    // 4. 调用处理函数
    srpc_php_set_header(&rpc_head);
    ret = srpc_php_handler_process(rpc_head.method_name(), extinfo, 1, req_body, rsp_body);
    if (ret != SRPC_SUCCESS)
    {
        RPC_REPORT(errmsg(ret));
        rpc_head.set_err(ret);
        msec_log_error(errmsg(ret));
        goto EXIT_LABEL;
    }

    // 5. 回包给前端
    SrpcPackPkg(&rsp_buff, &msg_len, &rpc_head, rsp_body);
    if (ret != SRPC_SUCCESS) {
        RPC_REPORT(errmsg(ret));
        msec_log_error(errmsg(ret));
        reset_log_option();
        return -1; 
    }   

    resblob.data = rsp_buff;
    resblob.len  = msg_len;
    commu->sendto(flow, &resblob, arg2);

    free(rsp_buff);
    reset_log_option();
    php_env_check();

    return 0;

EXIT_LABEL:

    // 6. 处理失败，组包并回包
    reset_log_option();

    ret = SrpcPackPkgNoBody(&rsp_buff, &msg_len, &rpc_head);
    if (ret != SRPC_SUCCESS) {
        RPC_REPORT(errmsg(ret));
        NGLOG_ERROR("pack package failed: %s",  errmsg(ret));
        return -1;
    }

    resblob.data = rsp_buff;
    resblob.len  = msg_len;
    commu->sendto(flow, &resblob, arg2);

    free(rsp_buff);
    php_env_check();

    return 0;  // 只要成功回包, 都返回0
}


/**
 * @brief 业务服务终止接口函数(proxy/worker)
 * @param server -业务进程信息
 */
extern "C" void spp_handle_fini(void* arg1, void* arg2)
{
    CServerBase* base = (CServerBase*)arg2;

    if (base->servertype() == SERVER_TYPE_WORKER )
    {
        srpc_fini();
    }
}

