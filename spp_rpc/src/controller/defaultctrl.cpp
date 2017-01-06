
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


#include "defaultctrl.h"
#include "../comm/tbase/misc.h"
#include "../comm/benchadapter.h"
#include "../comm/comm_def.h"
#include "../comm/global.h"
#include "../comm/singleton.h"
#include "../comm/keygen.h"
#include "../comm/monitor.h"
#include "../comm/tbase/misc.h"
#include "../comm/config/config.h"
#include "srpc_log.h"
#include <libgen.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>
#include <list>
#include <map>

#define NOTIFY_NUM   "notify_num"

#define PROXY_PID   "proxy_pid"
#define PROXY_FD    "proxy_fd"
#define PROXY_MEM   "proxy_mem"
#define PROXY_LOAD  "proxy_load"
#define PROXY_SRATE "proxy_srate"
#define PROXY_BRATE "proxy_brate"

using namespace spp::comm;
using namespace spp::ctrl;
using namespace spp::global;
using namespace spp::singleton;
using namespace spp::statdef;
using namespace std;
using namespace srpc;

#define SRPC_PID_FILE "./srpc.pid"

CDefaultCtrl::CDefaultCtrl()
: pidfile_fd_(0)
{
}

CDefaultCtrl::~CDefaultCtrl()
{
    if (pidfile_fd_ > 0)
    {
        close(pidfile_fd_);
        unlink(SRPC_PID_FILE);
    }

    for (unsigned i = 0; i < cmd_on_exit_.size(); ++i)
        system(cmd_on_exit_[i].c_str());
}

void CDefaultCtrl::dump_pid()
{
    static char buf[2][8192]; // 保存新旧pid list
    static unsigned idx = 0;
    int len = monsrv_.dump_pid_list(buf[idx], 8192);
    if (strncmp(buf[0], buf[1], len) != 0)
    {// 发生变化时写入文件
        flock(pidfile_fd_, LOCK_EX);
        ftruncate(pidfile_fd_, 0);
        lseek(pidfile_fd_, 0, SEEK_SET);
        write(pidfile_fd_, buf[idx], len);
        flock(pidfile_fd_, LOCK_UN);

        idx = (idx + 1) % 2; // swap
    }
}

void CDefaultCtrl::check_ctrl_running()
{
    FILE* file = fopen(SRPC_PID_FILE, "r");
    if (file == NULL) return;

    flock(fileno(file), LOCK_EX);

    size_t len = 0;
    char* line = NULL;
    int pid = 0;
    if(getline(&line, &len, file) != -1)
    {
        // line = "xxx [SPP_CTRL]"
        if (line)
        {
            char* pos = strstr(line, "[SPP_CTRL]");
            if (pos)
            {
                char pidstr[16] = {0};
                strncpy(pidstr, line, pos - line - 1);
                pid = atoi(pidstr);
            }
        }
    }
    if (line) free(line);

    flock(fileno(file), LOCK_UN);
    fclose(file);
	
	if (pid <= 0)
	{
		unlink(SRPC_PID_FILE);
	}

    if(pid > 0 && !CMisc::check_process_exist(pid))
    {
        unlink(SRPC_PID_FILE);
    }
}

void CDefaultCtrl::realrun(int argc, char* argv[])
{
    check_ctrl_running();
    // 放这里是因为，ctrl已经启动情况下，仍可以./spp_ctrl -v
    pidfile_fd_ = open(SRPC_PID_FILE, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (pidfile_fd_ == -1)
    {
        fprintf(stderr, "srpc_ctrl open "SRPC_PID_FILE" error:%m\n"
                        "srpc_ctrl is running? Check it now~ OR delete "SRPC_PID_FILE"\n\n");
        exit(-1);
    }

    fcntl(pidfile_fd_, F_SETFD, FD_CLOEXEC);
	
	dump_pid();
    clear_stat_file();

    //初始化配置
    SingleTon<CTLog, CreateByProto>::SetProto(&flog_);
    initconf(false);

    flog_.LOG_P_PID(LOG_FATAL, "controller started!\n");

    monsrv_.run(); // 让第一周期就有数据
    sleep(2);

    while (true)
    {
        //监控信息处理
        monsrv_.run();

        //检查reload信号
        //利用reload信号实现worker和proxy的热重载功能
        if (unlikely(CServerBase::reload()))
        {
            flog_.LOG_P_FILE(LOG_FATAL, "recv reload signal\n");

            //被监控的进程根据ctrl指令执行重启
            //ctrl读取热加载配置文件
            reloadconf();
            monsrv_.run();
        }

        //检查quit信号
        if (unlikely(CServerBase::quit()))
        {
            flog_.LOG_P_PID(LOG_FATAL, "recv quit signal\n");

            //停止所有被监控的进程
            monsrv_.killall(SIGUSR1);
            // 包脚本重启会等1s，而worker控制在500ms内安全退出
            usleep(500*1000);
            // 消灭残留进程
            monsrv_.killall(SIGKILL);
            break;
        }

        loop();

        dump_pid();
        sleep(3);
    }

    flog_.LOG_P_PID(LOG_FATAL, "controller stopped!\n");
}

// 框架循环调用
int CDefaultCtrl::loop()
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

        last = now;
        config_mtime = mtime;
    }

    return 0;
}

int CDefaultCtrl::initconf(bool reload)
{
    // 清理锁文件
    system("rm -rf ../bin/.mq_*.lock 2>/dev/null");

    // 初始化配置
    ConfigLoader loader;
    if (loader.Init(ix_->argv_[1]) != 0)
    {
        exit(-1);
    }

    Config& config = loader.GetConfig();

    set_servicename(config.service);

    // 初始化日志
    Log& flog = config.log;
    flog_.LOG_OPEN(flog.level, flog.type, "../log", "srpc_frame_ctrl", flog.maxfilesize, flog.maxfilenum);

    string progname = config.service;
    progname[progname.find(".")] = '_';

    // groupinfo配置: proxy配置
    TGroupInfo      groupinfo;
    memset(&groupinfo, 0, sizeof(groupinfo));
    groupinfo.adjust_proc_time  = 0;
    groupinfo.groupid_          = 0;
    groupinfo.exitsignal_       = SIGUSR1;
    groupinfo.maxprocnum_       = 1;
    groupinfo.minprocnum_       = 1;
    groupinfo.heartbeat_        = config.heartbeat;
    groupinfo.affinity_         = -1;

    snprintf(groupinfo.basepath_, (sizeof(groupinfo.basepath_) - 1), ".");
    snprintf(groupinfo.exefile_,  (sizeof(groupinfo.exefile_) - 1),  "srpc_%s_proxy", progname.c_str());
    snprintf(groupinfo.etcfile_,  (sizeof(groupinfo.etcfile_) - 1),  "%s", ix_->argv_[1]);

    if (!reload)
    {
        int ret = monsrv_.add_group(&groupinfo);
        if(0 != ret)
            LOG_SCREEN(LOG_ERROR, "ctrl add group[%d] error, ret[%d]\n", groupinfo.groupid_, ret);
    }
    else
        monsrv_.mod_group(groupinfo.groupid_, &groupinfo);

    // groupinfo配置: worker配置
    memset(&groupinfo, 0x0, sizeof(TGroupInfo));
    groupinfo.adjust_proc_time  = 0;
    groupinfo.groupid_          = 1;
    groupinfo.exitsignal_       = SIGUSR1;
    groupinfo.maxprocnum_       = config.procnum;
    groupinfo.minprocnum_       = config.procnum;
    groupinfo.heartbeat_        = config.heartbeat;
    groupinfo.affinity_         = -1;

    snprintf(groupinfo.basepath_, (sizeof(groupinfo.basepath_) - 1), ".");
    snprintf(groupinfo.exefile_,  (sizeof(groupinfo.exefile_) - 1),  "srpc_%s_worker", progname.c_str());
    snprintf(groupinfo.etcfile_,  (sizeof(groupinfo.etcfile_) - 1),  "%s", ix_->argv_[1]);

    // 清理共享内存
    key_t comsumer_key = pwdtok( 2 * groupinfo.groupid_ );
    key_t producer_key = pwdtok( 2 * groupinfo.groupid_ + 1 );

    char rm_cmd[64] = {0};
    snprintf( rm_cmd, sizeof( rm_cmd ), "ipcrm -M 0x%x 2>/dev/null", comsumer_key );
    cmd_on_exit_.push_back(rm_cmd);
    system( rm_cmd );
    snprintf( rm_cmd, sizeof( rm_cmd ), "ipcrm -M 0x%x 2>/dev/null", producer_key);
    cmd_on_exit_.push_back(rm_cmd);
    system( rm_cmd );

    if (!reload)
    {
        int ret = monsrv_.add_group(&groupinfo);
        if(0 != ret)
            LOG_SCREEN(LOG_ERROR, "ctrl add group[%d] error, ret[%d]\n", groupinfo.groupid_, ret);
    }
    else
        monsrv_.mod_group(groupinfo.groupid_, &groupinfo);

    // 初始化monitor上报
    CMonitorBase *monitor = new CNgseMonitor(config.service.c_str());
    if (monitor->init(NULL))
    {
        printf("\n[ERROR] monitor init failed!!\n");
        exit(-1);
    }

    MonitorRegist(monitor);

#if 0
    // groupinfo配置: worker配置
    for (WorkersConf::iterator it = workers.begin(); it != workers.end(); ++it)
    {
        WorkerConf& worker = it->second;
        memset(&groupinfo, 0x0, sizeof(TGroupInfo));
        groupinfo.adjust_proc_time  = 0;
        groupinfo.groupid_          = worker.id;
        groupinfo.exitsignal_       = SIGUSR1;
        groupinfo.maxprocnum_       = worker.procnum;
        groupinfo.minprocnum_       = worker.procnum;
        groupinfo.heartbeat_        = worker.interval * 3;
        groupinfo.affinity_         = -1;

        snprintf(groupinfo.basepath_, (sizeof(groupinfo.basepath_) - 1), ".");
        snprintf(groupinfo.exefile_,  (sizeof(groupinfo.exefile_) - 1),  "spp_%s_worker", global.service.c_str());
        snprintf(groupinfo.etcfile_,  (sizeof(groupinfo.etcfile_) - 1),  "%s", ix_->argv_[1]);

        // 清理共享内存
        key_t comsumer_key = pwdtok( 2 * groupinfo.groupid_ );
        key_t producer_key = pwdtok( 2 * groupinfo.groupid_ + 1 );

        char rm_cmd[64] = {0};
        snprintf( rm_cmd, sizeof( rm_cmd ), "ipcrm -M 0x%x 2>/dev/null", comsumer_key );
        cmd_on_exit_.push_back(rm_cmd);
        system( rm_cmd );
        snprintf( rm_cmd, sizeof( rm_cmd ), "ipcrm -M 0x%x 2>/dev/null", producer_key);
        cmd_on_exit_.push_back(rm_cmd);
        system( rm_cmd );

        if (!reload)
        {
            int ret = monsrv_.add_group(&groupinfo);
            if(0 != ret)
                LOG_SCREEN(LOG_ERROR, "ctrl add group[%d] error, ret[%d]\n", groupinfo.groupid_, ret);
        }
        else
            monsrv_.mod_group(groupinfo.groupid_, &groupinfo);
    }
#endif


    key_t mqkey = pwdtok(255);
    flog_.LOG_P_PID(LOG_INFO, "spp ctrl mqkey=%u\n", mqkey);

    if (mqkey == 0)
    {
        mqkey = DEFAULT_MQ_KEY;
    }

    snprintf(rm_cmd, sizeof(rm_cmd), "ipcrm -Q %d 2>/dev/null", mqkey);
    system(rm_cmd);

    CCommu* commu = new CMQCommu(mqkey);
    monsrv_.set_commu(commu);
    monsrv_.set_tlog(&flog_);

    return 0;
}
void CDefaultCtrl::clear_stat_file()
{
    char rm_cmd[256];
    snprintf(rm_cmd, sizeof(rm_cmd) - 1, "rm -rf ../stat/* 2>/dev/null");
    system(rm_cmd);
    return;
}


int CDefaultCtrl::reloadconf()
{
    ConfigLoader loader;
    if (loader.Init(ix_->argv_[1]) != 0)
    {
        return -1;
    }

    Config& config = loader.GetConfig();

    // groupinfo配置
    if (config.reload == 0)
    {
        return 0;
    }

    TGroupInfo groupinfo;
    memset(&groupinfo, 0x0, sizeof(TGroupInfo));
    groupinfo.adjust_proc_time  = 0;
    groupinfo.groupid_          = 1;
    groupinfo.exitsignal_       = SIGUSR1;
    groupinfo.maxprocnum_       = config.procnum;
    groupinfo.minprocnum_       = config.procnum;
    groupinfo.heartbeat_        = config.heartbeat;
    groupinfo.affinity_         = -1;

    snprintf(groupinfo.basepath_, (sizeof(groupinfo.basepath_) - 1), ".");
    //snprintf(groupinfo.exefile_,  (sizeof(groupinfo.exefile_) - 1),  "spp_%s_worker", config.service.c_str());
    snprintf(groupinfo.exefile_,  (sizeof(groupinfo.exefile_) - 1),  "spp_worker");
    snprintf(groupinfo.etcfile_,  (sizeof(groupinfo.etcfile_) - 1),  "%s", ix_->argv_[1]);

	// 设置状态为待热重启状态
	groupinfo.reload_       = PROC_RELOAD_START;
	groupinfo.reload_time   = time(NULL);

    monsrv_.pre_reload_group(groupinfo.groupid_, &groupinfo);

    return 0;
}
