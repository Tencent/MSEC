
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


//必须包含spp的头文件
#include "sppincl.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//格式化时间输出
char *format_time( time_t tm);

//初始化方法（可选实现）
//arg1:	配置文件
//arg2:	服务器容器对象
//返回0成功，非0失败
extern "C" int spp_handle_init(void* arg1, void* arg2)
{
    //插件自身的配置文件
    const char* etc = (const char*)arg1;
    //服务器容器对象
    CServerBase* base = (CServerBase*)arg2;

    base->log_.LOG_P_PID(LOG_DEBUG, "spp_handle_init, config:%s, servertype:%d\n", etc, base->servertype());

    return 0;
}

//数据接收（必须实现）
//flow:	请求包标志
//arg1:	数据块对象
//arg2:	服务器容器对象
//返回值：> 0 表示数据已经接收完整且该值表示数据包的长度
// == 0 表示数据包还未接收完整
// < 0 负数表示出错，将会断开连接
extern "C" int spp_handle_input(unsigned flow, void* arg1, void* arg2)
{
    //数据块对象，结构请参考tcommu.h
    blob_type* blob = (blob_type*)arg1;
    //extinfo有扩展信息
    TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;
    //服务器容器对象
    CServerBase* base = (CServerBase*)arg2;

    base->log_.LOG_P(LOG_DEBUG, "spp_handle_input[recv time:%s] flow:%d, buffer len:%d, client ip:%s\n",
                     format_time(extinfo->recvtime_),
                     flow,
                     blob->len,
                     inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));

    return blob->len;
}

//路由选择（可选实现）
//flow:	请求包标志
//arg1:	数据块对象
//arg2:	服务器容器对象
//返回值表示worker的组号
extern "C" int spp_handle_route(unsigned flow, void* arg1, void* arg2)
{
    //服务器容器对象
    CServerBase* base = (CServerBase*)arg2;
    base->log_.LOG_P_FILE(LOG_DEBUG, "spp_handle_route, flow:%d\n", flow);
    return 1;
}

//数据处理（必须实现）
//flow:	请求包标志
//arg1:	数据块对象
//arg2:	服务器容器对象
//返回0表示成功，非0失败（将会主动断开连接）
extern "C" int spp_handle_process(unsigned flow, void* arg1, void* arg2)
{
    //数据块对象，结构请参考tcommu.h
    blob_type* blob = (blob_type*)arg1;
    //数据来源的通讯组件对象
    CTCommu* commu = (CTCommu*)blob->owner;
    //extinfo有扩展信息
    TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;
    //服务器容器对象
    CServerBase* base = (CServerBase*)arg2;

    base->log_.LOG_P_PID(LOG_DEBUG, "spp_handle_process[recv time:%s] flow:%d, buffer len:%d, client ip:%s\n",
                         format_time(extinfo->recvtime_),
                         flow,
                         blob->len,
                         inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));

    //echo logic
    int ret = commu->sendto(flow, arg1, arg2 );
    if (ret != 0)
    {
        base->log_.LOG_P_PID(LOG_ERROR, "send response error, ret:%d\n", ret);
        return ret;
    }

    //if all responses were sent, send a NULL blob to release flow
    //blob_type release_cmd;
    //release_cmd.data = NULL;
    //release_cmd.len = 0;
    //commu->sendto(flow, &release_cmd, arg2);

    return 0;
}

//析构资源（可选实现）
//arg1:	保留参数
//arg2:	服务器容器对象
extern "C" void spp_handle_fini(void* arg1, void* arg2)
{
    //服务器容器对象
    CServerBase* base = (CServerBase*)arg2;

    base->log_.LOG_P_PID(LOG_DEBUG, "spp_handle_fini\n");
}

char *format_time( time_t tm)
{
    static char str_tm[1024];
    struct tm tmm;
    memset(&tmm, 0, sizeof(tmm) );
    localtime_r((time_t *)&tm, &tmm);

    snprintf(str_tm, sizeof(str_tm), "[%04d-%02d-%02d %02d:%02d:%02d]",
             tmm.tm_year + 1900, tmm.tm_mon + 1, tmm.tm_mday,
             tmm.tm_hour, tmm.tm_min, tmm.tm_sec);

    return str_tm;
}
