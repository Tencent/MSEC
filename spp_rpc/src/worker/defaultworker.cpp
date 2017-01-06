
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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include "defaultworker.h"
#include "misc.h"
#include "benchadapter.h"
#include "misc.h"
#include "global.h"
#include "singleton.h"

#include "comm_def.h"

#include "../comm/keygen.h"
#include "../comm/tbase/list.h"
#include "../comm/tbase/notify.h"
#include "../comm/tbase/tsockcommu/tsocket.h"
#include "../comm/tbase/misc.h"
#include "../comm/monitor.h"
#include "../comm/timestamp.h"
#include "../comm/config/config.h"
#include "../comm/shmcommu.h"
#include "exception.h"
#include "memlog.h"

#include "StatMgr.h"
#include "StatMgrInstance.h"
#include "srpc_log.h"
#include "SyncFrame.h"

#define WORKER_STAT_BUF_SIZE 1<<14

#define MAX_FILENAME_LEN 255

using namespace spp;
using namespace spp::comm;
using namespace spp::worker;
using namespace spp::global;
using namespace spp::singleton;
using namespace spp::statdef;
using namespace std;
using namespace tbase::notify;
using namespace srpc;

using namespace SPP_SYNCFRAME;
USING_SPP_STAT_NS;

extern CoreCheckPoint g_check_point;
extern struct sigaction g_sa;
extern void sigsegv_handler(int sig);
int g_spp_groupid;
int g_spp_groups_sum;

void exception_cb(int signo, void *arg)
{
    int     ret;
    key_t   mqkey = pwdtok(255);
    int     mqid  = msgget(mqkey, 0);

    TProcMonMsg *msg = (TProcMonMsg *)arg;

    msg->timestamp_ = time(NULL);
    msg->srctype_   = (signo << EXCEPTION_STARTBIT) | (MSG_VERSION << 1) | MSG_SRC_CLIENT;

    if (mqkey == 0) {
        mqkey = DEFAULT_MQ_KEY;
    }

    do {
        ret = msgsnd(mqid, (const void *)msg, msg->msglen_, IPC_NOWAIT);
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }

            // failure
        }
        break;
    } while (1);
}

CDefaultWorker::CDefaultWorker(): ator_(NULL), TOS_(-1), notify_fd_(0)
{
}

CDefaultWorker::~CDefaultWorker()
{
    if (ator_ != NULL)
        delete ator_;

}

// 获取是否启用微线程
bool CDefaultWorker::get_mt_flag()
{
    // 静态库和动态库版本的微线程同时存在，哪个版本有做初始化，则使用了微线程。
    bool flag = CSyncFrame::Instance()->GetMtFlag();
    if (flag)
    {
        return true;
    }

    if (sppdll.spp_mirco_thread && sppdll.spp_mirco_thread())
    {
        return true;
    }

    return false;
}

// 框架循环调用
int CDefaultWorker::loop()
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

        // 更新远程日志配置
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

// 微线程切换函数
void CDefaultWorker::handle_switch(bool block)
{
    static CSyncFrame* sync_frame = CSyncFrame::Instance();
    bool flag = sync_frame->GetMtFlag();

    // 2. switch to run micro-thread
    if (flag)  // 使用了动态库版本的微线程
    {
        sync_frame->HandleSwitch(block);
    }
    else if(sppdll.spp_handle_switch) // 使用了静态库版本的微线程
    {
        sppdll.spp_handle_switch(block);
    }
}

void CDefaultWorker::realrun(int argc, char* argv[])
{
    // 初始化配置
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);

    initconf(false);

    time_t  nowtime = time(NULL), montime = 0;
    int64_t now_ms = 0;

    flog_.LOG_P_PID(LOG_FATAL, "worker started!\n");

    while (true)
    {
        ///< start: micro thread handle loop entry add 20130715
        if (sppdll.spp_handle_loop)
        {
            sppdll.spp_handle_loop(this);   
        }            
        ///< end: micro thread handle loop entry 20130715

        // == 0 时，表示没取到请求，进入较长时间异步等待
        bool isBlock = (ator_->poll(false) == 0);

        static CSyncFrame* sync_frame = CSyncFrame::Instance();
        sync_frame->HandleSwitch(isBlock);

        // Check and reconnect net proxy, default 10 ms 
        now_ms = get_time_ms();

        // 检查quit信号
        if (unlikely(CServerBase::quit()) || unlikely(CServerBase::reload()))
        {
			now_ms = get_time_ms();
			
            // 保证剩下的请求都处理完
            if (unlikely(CServerBase::quit()))
            {        
	            flog_.LOG_P_PID(LOG_FATAL, "recv quit signal at %u\n", now_ms);
            	ator_->poll(true);
            }
			else
			{			
				flog_.LOG_P_PID(LOG_FATAL, "recv reload signal at %u\n", now_ms);
			}

			int timeout = 0;
			//微线程
			while (CSyncFrame::Instance()->GetThreadNum() > 1 && timeout < 1000)
			{
                CSyncFrame::Instance()->sleep(10000);
				timeout += 10;
			}
			
			now_ms = get_time_ms();
			flog_.LOG_P_PID(LOG_FATAL, "exit at %u\n", now_ms);
			break;
        }

        //监控信息上报
        nowtime = time(NULL);

        if ( unlikely(nowtime - montime > ix_->moni_inter_time_) )
        {
            CLI_SEND_INFO(&moncli_)->timestamp_ = nowtime;
            moncli_.run();
            montime = nowtime;
            flog_.LOG_P_PID(LOG_DEBUG, "moncli run!\n");
        }

        loop();
    }

    g_check_point = CoreCheckPoint_HandleFini;           // 设置待调用插件的CheckPoint
    if (sppdll.spp_handle_fini != NULL)
        sppdll.spp_handle_fini(NULL, this);
    g_check_point = CoreCheckPoint_SppFrame;                // 恢复CheckPoint，重置为CoreCheckPoint_SppFrame

    CStatMgrInstance::destroy();

    flog_.LOG_P_PID(LOG_FATAL, "worker stopped!\n");
}

void CDefaultWorker::assign_signal(int signo)
{
	switch (signo) {
		case SIGSEGV:
		case SIGFPE:
		case SIGILL:
		case SIGABRT:
		case SIGBUS:
		case SIGSYS:
		case SIGTRAP:
			signal(signo, sigsegv_handler);
			break;
		default:
			exit(1);
	}
}

int CDefaultWorker::initconf(bool reload)
{
    if ( reload )
    {
        return 0;
    }

    int pid = getpid();

    printf("\nWorker[%5d] init...\n", pid);

    groupid_         = 1;
    g_spp_groupid    = 1;
    g_spp_groups_sum = 1;
    g_spp_shm_fifo   = 0;
    TOS_             = 0;

    int ret = 0;

    ConfigLoader loader;
    if (loader.Init(ix_->argv_[1]) != 0)
    {
        exit(-1);
    }

    Config& config = loader.GetConfig();
    set_servicename(config.service);

    // 初始化monitor上报
    CMonitorBase *monitor = new CNgseMonitor(config.service.c_str());
    if (monitor->init())
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

    printf("Worker[%5d] Groupid = %d\n", pid, groupid_);

    CSyncFrame::Instance()->SetGroupId(groupid_);

    // 初始化框架日志
    char tmp[256];
    Log& flog = config.log;
    snprintf(tmp, 255, "srpc_frame_worker%d", groupid_);
    flog_.LOG_OPEN(flog.level, flog.type, "../log", tmp, flog.maxfilesize, flog.maxfilenum);

    // 初始化业务日志
    Log& log = config.log;
    snprintf(tmp, 255, "srpc_worker%d", groupid_);
    log_.LOG_OPEN(log.level, log.type, "../log", tmp, log.maxfilesize, log.maxfilenum);

    int notifyfd;
    spp_global::mem_full_warning_script = "";
    notifyfd = CNotify::notify_init( groupid_*2 + 1 ); // notify produce

    if (notifyfd < 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR]producer notify.groupid[%d]%m\n", groupid_);
        exit(-1);
    }

    TShmCommuConf shm;
    shm.shmkey_producer_ = pwdtok(groupid_ * 2 + 1);
    shm.shmkey_comsumer_ = pwdtok(groupid_ * 2);
    shm.locktype_ = LOCK_TYPE_PRODUCER | LOCK_TYPE_COMSUMER;
    shm.notifyfd_ = notifyfd;

    shm.shmsize_producer_ = config.shmsize * (1 << 20);
    shm.shmsize_comsumer_ = config.shmsize * (1 << 20) ;
    //  改变了语义，从2.7.0起，超时不在共享内存队列里进行
    msg_timeout_ = config.msg_timeout;

    LOG_SCREEN(LOG_ERROR, "Worker[%5d] [Shm]Proxy->Worker [%dMB]\n", pid, shm.shmsize_comsumer_/(1<<20));
    LOG_SCREEN(LOG_ERROR, "Worker[%5d] [Shm]Worker->Proxy [%dMB]\n", pid, shm.shmsize_producer_/(1<<20));

    // check commu type
    ator_ = new CTShmCommu;
    ret = ator_->init(&shm);

    if (ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR]Worker acceptor init error, return %d\n", ret);
        exit(-1);
    }

    ator_->reg_cb(CB_SENDDATA, ator_senddata, this);
    ator_->reg_cb(CB_OVERLOAD, ator_overload, this);
    ator_->reg_cb(CB_SENDERROR, ator_senderror, this);

    if (spp_global::mem_full_warning_script != "")
    {
        struct stat fstat;

        if (stat(spp_global::mem_full_warning_script.c_str(), &fstat) == -1 ||
                (fstat.st_mode & S_IFREG) == 0)
        {
            fprintf( stderr, "[WARNING] %d Attribute scriptname error in config file %s.\n",  __LINE__, ix_->argv_[1]);
            spp_global::mem_full_warning_script.clear();
        }
    }


    //初始化框架统计
    snprintf(tmp, 255, "../stat/stat_srpc_worker%d.dat", groupid_);
    int stat_ret = fstat_.init_statpool(tmp);
    if (stat_ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "statpool init error, ret:%d, errmsg:%m\n", stat_ret);
        exit(-1);
    }

    fstat_.init_statobj_frame(SHM_RX_BYTES, STAT_TYPE_SUM, WIDX_SHM_RX_BYTES,
            "接收包量/字节数");
    fstat_.init_statobj_frame(SHM_TX_BYTES, STAT_TYPE_SUM, WIDX_SHM_TX_BYTES,
            "发送包量/字节数");
    fstat_.init_statobj_frame(MSG_TIMEOUT, STAT_TYPE_SUM, WIDX_MSG_TIMEOUT,
            "Proxy请求丢弃数");
    fstat_.init_statobj_frame(MSG_SHM_TIME, STAT_TYPE_SUM, WIDX_MSG_SHM_TIME,
            "proxy->worker 请求接收到开始处理时延/毫秒");

    fstat_.init_statobj_frame(MT_THREAD_NUM, STAT_TYPE_SET, WIDX_MT_THREAD_NUM,
            "微线程数量");
    fstat_.init_statobj_frame(MT_MSG_NUM, STAT_TYPE_SET, WIDX_MT_MSG_NUM,
            "微线程消息数");

    //初始化业务统计
    snprintf(tmp, 255, "../stat/module_stat_srpc_worker%d.dat", groupid_);
    if (stat_.init_statpool(tmp) != 0)
    {
        LOG_SCREEN(LOG_ERROR, "statpool init error.\n");
        exit(-1);
    }

    //初始化监控
    int interval = config.heartbeat/3;
    ix_->moni_inter_time_ = interval<=3?3:interval;


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
    CLI_SEND_INFO(&moncli_)->procid_ = getpid();
    CLI_SEND_INFO(&moncli_)->timestamp_ = time(NULL);
    CLI_SEND_INFO(&moncli_)->reserve_[0] = 1;//资源尚未就绪，等待spp_handle_init
    moncli_.run();      // 避免spp_handle_init运行很久导致ctrl不断拉起进程的问题    
    CLI_SEND_INFO(&moncli_)->reserve_[0] = 0; //等待spp_handle_init，下次上报时，资源将就绪

    if (install_signal_handler("/var/srpc_exception", exception_cb, &(moncli_.msg_[0])) < 0)
    {
        LOG_SCREEN(LOG_ERROR, "install_signal_handler error.\n");
    }

    string module_file;
    string module_etc;
    bool module_isGlobal = false;
    module_file     = config.module;
    module_etc      = config.conf;
    module_isGlobal = (bool)config.global;

    LOG_SCREEN(LOG_ERROR, "Worker[%5d] Load module[%s] etc[%s]\n", pid, module_file.c_str(), module_etc.c_str());

    // 初始化基于命令字和错误码的统计对象
    char stat_mgr_file_name[256];
    snprintf(stat_mgr_file_name, sizeof(stat_mgr_file_name) - 1, SPP_STAT_MGR_FILE, g_spp_groupid);
    ret = CStatMgrInstance::instance()->initStatPool(stat_mgr_file_name);
    if (ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR] init cost stat pool errno = %d\n", ret);
        exit(-1);
    }

    // 初始化微线程框架
    ret = CSyncFrame::Instance()->InitFrame(this, 10000);
    if (ret < 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR] init syncframe failed = %d\n", ret);
        exit(-1);
    }

    if (0 == load_bench_adapter(module_file.c_str(), module_isGlobal))
    {
        LOG_SCREEN(LOG_FATAL, "call spp_handle_init ...\n");
        int handle_init_ret = 0;

        g_check_point = CoreCheckPoint_HandleInit;
        handle_init_ret = sppdll.spp_handle_init((void*)module_etc.c_str(), this);
        g_check_point = CoreCheckPoint_SppFrame;

        if (handle_init_ret != 0)
        {
            LOG_SCREEN(LOG_ERROR, "Worker module %s handle init error, return %d\n",
                       module_file.c_str(), handle_init_ret);
            exit(-1);
        }

        ator_->reg_cb(CB_RECVDATA, ator_recvdata_v2, this);	//数据接收回调注册
    }
    else
    {
        LOG_SCREEN(LOG_ERROR, "Worker load module:%s failed.\n", module_file.c_str());
        exit(-1);
    }

    // 初始化后，即可知道是否异步
    fstat_.init_statobj("worker_type", STAT_TYPE_SET, "微线程[3]/异步[2]/同步[1]类型");
    if (get_mt_flag())
        fstat_.step0("worker_type", 3);
    else
        fstat_.step0("worker_type", 1);

    ret =  CStatMgrInstance::instance()->initFrameStatItem();
    if (ret != 0)
    {
        LOG_SCREEN(LOG_ERROR, "[ERROR] init frame stat errno = %d\n", ret);
        exit(-1);
    }

    LOG_SCREEN(LOG_ERROR, "Worker[%5d] OK!\n", pid);

    return 0;
}

int CDefaultWorker::ator_recvdata_v2(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultWorker* worker = (CDefaultWorker*)arg2;
    worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator recvdata v2, flow:%u,blob len:%d\n", flow, blob->len);

    if (likely(blob->len > 0))
    {
        TConnExtInfo* ptr = NULL;

        MONITOR(MONITOR_WORKER_FROM_PROXY);
        blob->len -= sizeof(TConnExtInfo);
        blob->extdata = blob->data + blob->len;
        ptr = (TConnExtInfo*)blob->extdata;

        int64_t recv_ms = int64_t(ptr->recvtime_) * 1000 + ptr->tv_usec / 1000;
        int64_t now = get_time_ms();

        int64_t time_delay = now - recv_ms;

        worker->fstat_.op(WIDX_MSG_SHM_TIME, time_delay);
        add_memlog(blob->data, blob->len);

        if( worker->msg_timeout_ )
        {        
			shm_delay_stat(time_delay); 
            if ( time_delay > worker->msg_timeout_ )
            {
                MONITOR(MONITOR_WORKER_OVERLOAD_DROP);
                worker->fstat_.op(WIDX_MSG_TIMEOUT, 1);
                worker->flog_.LOG_P_PID(LOG_ERROR, "Flow[%u] Msg Timeout! Delay[%d], Drop!\n"
                    , flow, int(time_delay), blob->len);

                if (UDP_SOCKET == ptr->type_)
                {
                    CTCommu* commu = (CTCommu*)blob->owner;
                    blob_type rspblob;
                    rspblob.len = 0;
                    rspblob.data = NULL;
                    commu->sendto(flow, &rspblob, NULL);
                    worker->flog_.LOG_P_FILE(LOG_DEBUG, "close conn, flow:%u\n", flow);
                }

                return 0;
            }
        }

        worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator recvdone, flow:%u, blob len:%d\n", flow, blob->len);
        worker->fstat_.op(WIDX_SHM_RX_BYTES, blob->len); // 累加接收字节数

        g_check_point = CoreCheckPoint_HandleProcess;           // 设置待调用插件的CheckPoint
        int ret = sppdll.spp_handle_process(flow, arg1, arg2);
        g_check_point = CoreCheckPoint_SppFrame;                // 恢复CheckPoint，重置为CoreCheckPoint_SppFrame

        if (likely(!ret))
        {
            MONITOR(MONITOR_WORKER_PROC_SUSS);
            return 0;
        }
        else
        {
            MONITOR(MONITOR_WORKER_PROC_FAIL);
            CTCommu* commu = (CTCommu*)blob->owner;
            blob_type rspblob;

            rspblob.len = 0;
            rspblob.data = NULL;

            commu->sendto(flow, &rspblob, NULL);
            worker->flog_.LOG_P_FILE(LOG_DEBUG, "close conn, flow:%u\n", flow);
        }
    }

    return -1;
}

int CDefaultWorker::ator_senddata(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultWorker* worker = (CDefaultWorker*)arg2;
    worker->flog_.LOG_P_FILE(LOG_DEBUG, "ator senddata, flow:%u, blob len:%d\n", flow, blob->len);
    if (blob->len > 0)
    {
        worker->fstat_.op(WIDX_SHM_TX_BYTES, blob->len); // 累加回包字节数
    }

    return 0;
}

int CDefaultWorker::ator_overload(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultWorker* worker = (CDefaultWorker*)arg2;
    worker->flog_.LOG_P_PID(LOG_ERROR, "worker overload, blob->data: %d\n", (long)blob->data);

    return 0;
}

//add by jeremy
int CDefaultWorker::ator_senderror(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;
    CDefaultWorker* worker = (CDefaultWorker*)arg2;
    worker->flog_.LOG_P_PID(LOG_DEBUG, "ator send error, flow[%u], len[%d]\n", flow, blob->len);

    return 0;
}

