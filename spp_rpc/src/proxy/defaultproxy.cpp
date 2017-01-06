
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


#include <libgen.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "defaultproxy.h"
#include "../comm/tbase/misc.h"
#include "../comm/comm_def.h"
#include "../comm/benchadapter.h"
#include "../comm/global.h"
#include "../comm/monitor.h"
#include "../comm/singleton.h"
#include "../comm/keygen.h"
#include "../comm/tbase/misc.h"
#include "../comm/tbase/misc.h"
#include "../comm/tbase/notify.h"
#include "../comm/config/config.h"
#include "../comm/shmcommu.h"
#include "../comm/timestamp.h"
#include "srpc_log.h"

#define PROXY_STAT_BUF_SIZE 1<<14

#define DEFAULT_ROUTE_NO  1

using namespace spp::comm;
using namespace spp::proxy;
using namespace spp::global;
using namespace spp::singleton;
using namespace spp::statdef;
using namespace tbase::notify;
using namespace srpc;

int g_spp_groupid;
int g_spp_groups_sum;

class mypair
{
public:
    mypair(int a, int b): first(a), second(b) {}
    int first;
    int second;
    bool operator < (const mypair &m)const
    {
        return first < m.first;
    }
};

CDefaultProxy::CDefaultProxy(): ator_(NULL), iplimit_(IPLIMIT_DISABLE)
{
}

CDefaultProxy::~CDefaultProxy()
{
    if (ator_ != NULL)
        delete ator_;

    map<int, CTCommu*>::iterator it;
    for (it = ctor_.begin(); it != ctor_.end(); it++)
    {
        if(it->second != NULL)
            delete it->second;
    }
}

void CDefaultProxy::realrun(int argc, char* argv[])
{
    //初始化配置
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    initconf(false);
    ix_->fstat_inter_time_ = 5; //5秒 统计一次框架信息
    time_t fstattime = 0;
    time_t montime = 0;
    static char statbuff[PROXY_STAT_BUF_SIZE];
    char* pstatbuff = (char*)statbuff;
    int	bufflen = 0;
    char ctor_stat[1 << 16] = {0};
    int ctor_stat_len = 0;
    int processed = 0;

    flog_.LOG_P_PID(LOG_FATAL, "proxy started!\n");

    while (true)
    {

        //轮询acceptor
        ator_->poll(processed == 0); // need sleep ?
        processed = 0;

        //轮询connector
        map<int, CTCommu*>::iterator it;
        for (it = ctor_.begin(); it != ctor_.end(); it++)
        {
            spp_global::set_cur_group_id(it->first);
            processed += it->second->poll(true); // block
        }
        if (unlikely(get_time_s() - fstattime > ix_->fstat_inter_time_))
        {
            fstattime = get_time_s();
            fstat_.op(PIDX_CUR_CONN, ((CTSockCommu*)ator_)->getconn());
        }
        if ( unlikely(get_time_s() - montime > ix_->moni_inter_time_) )
        {

            // 上报信息到ctrl进程
            CLI_SEND_INFO(&moncli_)->timestamp_ = get_time_s();
            moncli_.run();
            montime = get_time_s();
            flog_.LOG_P_PID(LOG_DEBUG, "moncli run!\n");

            //输出监控信息
            fstat_.result(&pstatbuff, &bufflen, PROXY_STAT_BUF_SIZE);
            monilog_.LOG_P_NOTIME(LOG_INFO, "%s", statbuff);

            //输出ctor的统计信息
            ctor_stat_len = 0;
            ctor_stat_len += snprintf(ctor_stat, sizeof(ctor_stat), "Connector Stat\n");

            for (it = ctor_.begin(); it != ctor_.end(); it++)
            {
                it->second->ctrl(0, CT_STAT, ctor_stat, &ctor_stat_len);
            }

            //flog_.LOG_P_NOTIME(LOG_INFO, "%s\n", ctor_stat);
        }

        //检查quit信号
        if (unlikely(CServerBase::quit()))
        {
            flog_.LOG_P_PID(LOG_FATAL, "recv quit signal\n");
            int count = 20;
            while(count--)
            {
                for (it = ctor_.begin(); it != ctor_.end(); it++)
                {
                    it->second->poll(true); // block
                }
                usleep(20000);
            }
            break; // 20ms * 20
        }

        loop();
    }

    if (sppdll.spp_handle_fini != NULL)
        sppdll.spp_handle_fini(NULL, this);

    flog_.LOG_P_PID(LOG_FATAL, "proxy stopped!\n");
}

unsigned CDefaultProxy::GetListenIp(const char *intf)
{
    if (NULL == intf || intf[0] == '\0')
    {
        return 0U;
    }

    return CMisc::getip(intf);
}

// 框架循环调用
int CDefaultProxy::loop()
{
    static int64_t config_mtime;
    static uint64_t last = 0;
    uint64_t now = (uint64_t)time(NULL);

    if (now > (last + 5))
    {
        int64_t mtime;
        mtime = CMisc::get_file_mtime(ix_->argv_[1]);
        if ((mtime < 0) || (mtime == config_mtime))
        {
            last = now;
            return 0;
        }
        
        // 更新本地日志级别
        ConfigLoader loader;
        if (loader.Init(ix_->argv_[1]) != 0)
        {
            //exit(-1);
            return -1;
        }
        
        Config& config = loader.GetConfig();

        flog_.log_level(config.log.level);
        log_.log_level(config.log.level);

        // 更新远程日志
        CNgLogAdpt *rlog = (CNgLogAdpt *)GetRlog();
        if (rlog->Init(ix_->argv_[1], servicename().c_str()))
        {
            flog_.LOG_P_PID(LOG_INFO, "Ngse rlog reload failed\n");
        }

        last = now;
        config_mtime = mtime;
    }

    return 0;
}


int CDefaultProxy::initconf(bool reload)
{
    if (reload)
    {
        return 0;
    }

    int pid = getpid();

    printf("\nProxy[%5d] init...\n", pid);

    ConfigLoader loader;
    if (loader.Init(ix_->argv_[1]) != 0)
    {
        exit(-1);
    }

    Config& config = loader.GetConfig();

    set_servicename(config.service);

    // 初始化monitor上报
    CMonitorBase *monitor = new CNgseMonitor(config.service.c_str());
    if (monitor->init(NULL))
    {
        printf("\n[ERROR] monitor init failed!!\n");
        exit(-1);
    }

    MonitorRegist(monitor);

    // 初始化SRPC远程日志
    CNgLogAdpt *rlog = new CNgLogAdpt;
    if (rlog->Init(ix_->argv_[1], config.service.c_str()))
    {
        printf("\n[ERROR] Ngse remote log init failed!!\n");
        //exit(-1);
    }

    RegisterRLog(rlog);

    // 初始化SRPC本地日志
    CSppLogAdpt *llog = new CSppLogAdpt(this);
    RegisterLlog(llog);

    groupid_       = 0;
    g_spp_groupid  = 0;
    g_spp_shm_fifo = 0;

    // 初始化日志
    Log& flog = config.log;
    flog_.LOG_OPEN(flog.level, flog.type, "../log", "srpc_frame_proxy", flog.maxfilesize, flog.maxfilenum);

    // 初始化业务日志
    Log& log = config.log;
    log_.LOG_OPEN(log.level, log.type, "../log", "srpc_proxy", log.maxfilesize, log.maxfilenum);

    // 初始化acceptor
    TSockCommuConf socks;
    memset(&socks, 0x0, sizeof(TSockCommuConf));

    socks.udpautoclose_ = true;
    socks.maxconn_      = 100000;
    socks.maxpkg_       = 100000;
    socks.expiretime_   = config.timeout;
    socks.sendcache_limit_ = -1;

    for (size_t i = 0; i < config.listens.size(); ++ i)
    {
        Listen& listen = config.listens[i];
        TSockBind binding;
        if (listen.type == "tcp") { // tcp
            binding.type_ = SOCK_TYPE_TCP;
            binding.oob_  = listen.oob;
        }
        else{ // udp
            binding.type_ = SOCK_TYPE_UDP;
            binding.oob_  = 0;//udp no oob
        }

        binding.ipport_.ip_   = GetListenIp(listen.intf.c_str());
        binding.ipport_.port_ = (unsigned short)listen.port;
        binding.TOS_          = 0;
        LOG_SCREEN(LOG_ERROR, "Proxy[%5d] Bind On [%s][%d]...\n",
                   pid, listen.type.c_str(), listen.port);

        socks.sockbind_.push_back(binding);
    }



    ator_ = new CTSockCommu;

    int ator_ret = ator_->init(&socks);
    if (ator_ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR]Proxy acceptor init error, return %d.\n", ator_ret);
        exit(-1);
    }

    ator_->reg_cb(CB_OVERLOAD, ator_overload, this);
    ator_->reg_cb(CB_DISCONNECT, ator_disconnect, this);

    // connector配置
    vector<TShmCommuConf> shms;
    g_spp_groups_sum = 1;

    
    TShmCommuConf shm;
    int groupid = 1;
    shm.shmkey_producer_  = pwdtok(groupid * 2);
    shm.shmsize_producer_ = config.shmsize* (1 << 20);

    shm.shmkey_comsumer_  = pwdtok(groupid * 2 + 1);
    shm.shmsize_comsumer_ =  config.shmsize * ( 1 << 20);

    shm.locktype_ = 0;      //proxy只有一个进程，读写都不需要加锁
    shm.maxpkg_ = 0;
    shm.groupid = groupid;
    shm.notifyfd_ = CNotify::notify_init(/* producer key */groupid * 2);
    shms.push_back(shm);

    if (((CTSockCommu *)ator_)->addnotify(groupid) < 0 || shm.notifyfd_ < 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR]proxy notify fifo init error.groupid:%d\n", groupid);
        exit(-1);
    }

    LOG_SCREEN(LOG_ERROR, "Proxy[%5d] [Shm]Proxy->WorkerGroup[%d] [%dMB]...\n", pid, groupid, shm.shmsize_producer_/(1<<20));
    LOG_SCREEN(LOG_ERROR, "Proxy[%5d] [Shm]WorkerGroup[%d]->Proxy [%dMB]...\n", pid, groupid, shm.shmsize_comsumer_/(1<<20));


    for (unsigned i = 0; i < shms.size(); ++i)
    {
        CTCommu* commu = NULL;
        commu = new CTShmCommu;
        int commu_ret = 0;
        commu_ret = commu->init(&shms[i]);

        if (commu_ret != 0)
        {
            LOG_SCREEN(LOG_ERROR, "[ERROR]Proxy CTShmCommu init error, return %d.\n", commu_ret);
            exit(-1);
        }
        commu_ret = commu->clear();	//clear shm of producer and comsumer

        if (commu_ret != 0)
        {
            LOG_SCREEN(LOG_ERROR, "[ERROR]Proxy CTShmCommu clear error, return %d.\n", commu_ret);
            exit(-1);
        }

        commu->reg_cb(CB_RECVDATA, ctor_recvdata, this);
        ctor_[shms[i].groupid] = commu;
    }


    // 初始化框架统计
    int stat_ret = fstat_.init_statpool("../stat/stat_srpc_proxy.dat");
    if (stat_ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "statpool init error, ret:%d, errmsg:%m\n", stat_ret);
        exit(-1);
    }

    fstat_.init_statobj_frame(RX_BYTES, STAT_TYPE_SUM, PIDX_RX_BYTES,
                              "接收 包量/字节数");
    fstat_.init_statobj_frame(TX_BYTES, STAT_TYPE_SUM, PIDX_TX_BYTES,
                              "发送 包量/字节数");
    fstat_.init_statobj_frame(CONN_OVERLOAD, STAT_TYPE_SUM, PIDX_CONN_OVERLOAD,
                              "接入连接失败数");
    fstat_.init_statobj_frame(SHM_ERROR, STAT_TYPE_SUM, PIDX_SHM_ERROR,
                              "共享内存错误次数");
    fstat_.init_statobj_frame(CUR_CONN, STAT_TYPE_SET, PIDX_CUR_CONN,
                              "接入proxy的连接数(tcp和udp协议)");

    // 初始化业务统计
    if (stat_.init_statpool("../stat/module_stat_srpc_proxy.dat") != 0)
    {
        LOG_SCREEN(LOG_ERROR, "statpool init error.\n");		//modified by jeremy
        exit(-1);
    }

    // 初始化监控
    int interval = config.heartbeat/3;
    ix_->moni_inter_time_ = interval>3?interval:3;

    // 初始化与Ctrl通信的MessageQueue
    int mqkey = pwdtok(255);
    if (mqkey == 0)
    {
        mqkey = DEFAULT_MQ_KEY;
    }

    CCommu* commu = new CMQCommu(mqkey);
    moncli_.set_commu(commu);
    memset(CLI_SEND_INFO(&moncli_), 0x0, sizeof(TProcInfo));
    CLI_SEND_INFO(&moncli_)->groupid_ = groupid_;
    CLI_SEND_INFO(&moncli_)->procid_  = getpid();
    CLI_SEND_INFO(&moncli_)->timestamp_ = time(NULL);
    moncli_.run();      // 避免spp_handle_init运行很久导致ctrl不断拉起进程的问题

    // 加载用户服务模块
    string module_file;
    string module_etc;
    module_file = config.module;
    module_etc  = config.conf;
    bool module_isGlobal = (bool)config.global;

    LOG_SCREEN(LOG_ERROR, "Proxy[%d] Load module[%s] etc[%s]...\n", pid, module_file.c_str(), module_etc.c_str());

    if (0 == load_bench_adapter(module_file.c_str(), module_isGlobal))
    {
        int handle_init_ret = 0;
        handle_init_ret = sppdll.spp_handle_init((void*)module_etc.c_str(), this);

        if (handle_init_ret != 0)
        {
            LOG_SCREEN(LOG_ERROR, "spp proxy module:%s handle init error, return %d\n",
                       module_file.c_str(), handle_init_ret);
            exit(-1);
        }

        ator_->reg_cb(CB_RECVDATA, ator_recvdata_v2, this);	//数据接收回调注册
    }
    else
    {
        LOG_SCREEN(LOG_ERROR, "Proxy load module:%s failed.\n", module_file.c_str());
        exit(-1);
    }

    return 0;
}
//一些回调函数

int CDefaultProxy::ator_overload(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultProxy* proxy = (CDefaultProxy*)arg2;
    proxy->fstat_.op(PIDX_CONN_OVERLOAD, 1);
    proxy->flog_.LOG_P_FILE(LOG_ERROR, "proxy overload %d\n", (long)blob->data);

    return 0;
}

int CDefaultProxy::ctor_recvdata(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultProxy* proxy = (CDefaultProxy*)arg2;
    proxy->flog_.LOG_P_FILE(LOG_DEBUG, "flow:%u,blob len:%d\n", flow, blob->len);

    MONITOR(MONITOR_WORKER_TO_PROXY);
    int ret = proxy->ator_->sendto(flow, arg1, NULL);

    if (likely(ret >= 0))
    {
        proxy->fstat_.op(PIDX_TX_BYTES, blob->len); // 累加回包字节数
    }

    return 0;
}

int CDefaultProxy::ator_recvdata_v2(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultProxy* proxy = (CDefaultProxy*)arg2;
    proxy->flog_.LOG_P_FILE(LOG_DEBUG, "flow:%u,blob len:%d\n", flow, blob->len);
    int total_len = blob->len;
    int processed_len = 0;
    int proto_len = -1;
    int ret = 0;
    int handle_exception = 0;

    while (blob->len > 0 && (proto_len = sppdll.spp_handle_input(flow, arg1, arg2)) > 0)
    {
        if (proto_len > blob->len)
        {
            proxy->flog_.LOG_P_FILE(LOG_ERROR,
                                    "spp_handle_input error, flow:%u, blob len:%d, proto_len:%d\n",
                                    flow, blob->len, proto_len);
            processed_len = total_len;
            break;
        }

        ret = 0;
        MONITOR(MONITOR_PROXY_PROC);
        proxy->fstat_.op(PIDX_RX_BYTES, proto_len); // 累加收包字节数
        processed_len += proto_len;

        unsigned route_no;

        if (!sppdll.spp_handle_route)
        {
            route_no = 1;
        }
        else
        {
            // 0x7FFF 为了去掉GROUPID并兼容
            route_no = sppdll.spp_handle_route(flow, arg1, arg2) & 0x7FFF;
        }

        proxy->flog_.LOG_P_FILE(LOG_DEBUG,
                                "ator recvdone, flow:%u, blob len:%d, proto_len:%d, route_no:%d\n",
                                flow, blob->len, proto_len, route_no);
        blob_type sendblob;

        static int datalen = 0;
        static char* data = NULL;
        int need_len = proto_len + sizeof(TConnExtInfo);
        if (datalen < need_len)
        {
            // 每次分配2倍需求空间
            data = (char*)CMisc::realloc_safe(data, 2 * need_len);// 兼容data == NULL
            datalen = 2 * need_len;
        }

        if (data != NULL)
        {
            memcpy(data, blob->data, proto_len);
            memcpy(data + proto_len, blob->extdata, sizeof(TConnExtInfo));
            sendblob.data = data;
            sendblob.len = proto_len + sizeof(TConnExtInfo);
        }
        else
        {
            datalen = 0;
            return proto_len;
        }

        map<int, CTCommu*>::iterator iter;
        if ((iter = proxy->ctor_.find(route_no)) != proxy->ctor_.end())
        {
            MONITOR(MONITOR_PROXY_TO_WORKER);
            ret = iter->second->sendto(flow, &sendblob, NULL);
        }
        else
        {
            proxy->flog_.LOG_P_FILE(LOG_ERROR,
                    "group route to %d error, flow:%u, blob len:%d, proto_len:%d\n",
                    route_no, flow, blob->len, proto_len);
        }

        if (unlikely(ret))
        {
            proxy->fstat_.op(PIDX_SHM_ERROR, 1); // 累加共享内存错误次数

            proxy->flog_.LOG_P_FILE(LOG_ERROR,
                        "send to worker error, ret:%d, route_no:%u, flow:%u, blob len:%d, proto_len:%d\n",
                        ret, route_no, flow, blob->len, proto_len);

            //加入业务异常回调控制,当业务认为该异常需要处理的时候，关闭连接
            if(sppdll.spp_handle_exception) {
                handle_exception = sppdll.spp_handle_exception(flow, arg1, arg2);

                if(handle_exception < 0)
                    return handle_exception;
            }
            break;
        }

        blob->data += proto_len;
        blob->len -= proto_len;
    }

    if (proto_len < 0)
    {
        return proto_len;
    }

    return processed_len;
}


int CDefaultProxy::ator_connected(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultProxy* proxy = (CDefaultProxy*)arg2;
    TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;

    if (proxy->iplimit_ == IPLIMIT_WHITE_LIST)
    {
        if (proxy->iptable_.find(extinfo->remoteip_) == proxy->iptable_.end())
        {
            proxy->flog_.LOG_P_FILE(LOG_ERROR,
                                    "ip white limited, %s\n",
                                    inet_ntoa(*((struct in_addr*)&extinfo->remoteip_)));
            return -1;
        }
    }
    else if (proxy->iplimit_ == IPLIMIT_BLACK_LIST)
    {
        if (proxy->iptable_.find(extinfo->remoteip_) != proxy->iptable_.end())
        {
            proxy->flog_.LOG_P_FILE(LOG_ERROR,
                                    "ip black limited, %s\n",
                                    inet_ntoa(*((struct in_addr*)&extinfo->remoteip_)));
            return -2;
        }
    }

    return 0;
}
// 返回值：0 丢弃数据（默认行为）
//         1 保留数据
int CDefaultProxy::ator_disconnect(unsigned flow, void* arg1, void* arg2)
{
    int ret = 0;
    blob_type* blob = (blob_type*)arg1;
    CDefaultProxy* proxy = (CDefaultProxy*)arg2;

    proxy->flog_.LOG_P_FILE(LOG_DEBUG, "flow:%u,blob len:%d\n", flow, blob->len);
    if (sppdll.spp_handle_close == NULL)
    {
        return 0;
    }

    if(blob->len == 0)
    {
        return 0;
    }
    int need_retain = sppdll.spp_handle_close(flow, arg1, arg2);
    if(need_retain == 0)
    {
        return 0;
    }

    proxy->fstat_.op(PIDX_RX_BYTES, blob->len); // 累加收包字节数

    unsigned route_no;

    if (!sppdll.spp_handle_route)
    {
        route_no = 1;
    }
    else
    {
        // 0x7FFF 为了去掉GROUPID并兼容
        route_no = sppdll.spp_handle_route(flow, arg1, arg2) & 0x7FFF;
    }

    proxy->flog_.LOG_P_FILE(LOG_DEBUG, "flow:%u, blob len:%d, route_no:%d\n", flow, blob->len, route_no);

    blob_type sendblob;

    static int datalen = 0;
    static char* data = NULL;
    int need_len = blob->len + sizeof(TConnExtInfo);
    if (datalen < need_len)
    {
        //每次分配2倍需求空间
        data = (char*)CMisc::realloc_safe(data, 2 * need_len);// 兼容data == NULL
        datalen = 2 * need_len;
    }

    if (data != NULL)
    {
        memcpy(data, blob->data, blob->len);
        memcpy(data + blob->len, blob->extdata, sizeof(TConnExtInfo));
        sendblob.data = data;
        sendblob.len = blob->len + sizeof(TConnExtInfo);
    }
    else
    {
        datalen = 0;
        return 0;
    }

    map<int, CTCommu*>::iterator iter;
    if ((iter = proxy->ctor_.find(route_no)) != proxy->ctor_.end())
    {
        ret = iter->second->sendto(flow, &sendblob, NULL);
    }
    else
    {
        proxy->flog_.LOG_P_FILE(LOG_ERROR, "group route to %d error, flow:%u, blob len:%d\n", route_no, flow, blob->len);
    }

    if (unlikely(ret))
    {
        proxy->fstat_.op(PIDX_SHM_ERROR, 1); // 累加共享内存错误次数
    }

    blob->data += blob->len;
    blob->len -= blob->len;
    return 1;
}

