
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
//#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../singleton.h"
#include "tprocmon.h"
#include "../serverbase.h"
#include "../monitor.h"
#include "../../proxy/defaultproxy.h"
#include "../../worker/defaultworker.h"
#include "misc.h"
#include "../global.h"
#include "hide_private_tp.h"

#include "../comm/keygen.h"



#define GROUP_UNUSED   -1

#define DO_LOG_PROCMON if(log_) log_->LOG_P_PID

using namespace tbase::tprocmon;
using namespace spp::singleton;
using namespace spp::global;
using namespace spp;
using namespace std;
using namespace tbase::tlog;
using namespace spp::worker;
using namespace spp::proxy;
using namespace spp::statdef;
using namespace spp::comm;

//static bool unused = do_recv(0); //recv all message at start, discard some expired client message

CMQCommu::CMQCommu(): mqid_(0)
{
}
CMQCommu::CMQCommu(int mqkey): mqid_(0)
{
    init((void*)&mqkey);
}

CMQCommu::~CMQCommu()
{
}

int CMQCommu::init(void* args)
{
    int mqkey = *((int*)args);
    mqid_ = msgget(mqkey, IPC_CREAT | 0666);

    if (mqid_ < 0)
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "message queue create failed, mqkey = %d, msgget[%m]\n", mqkey);
        printf("[ERROR]message queue create failed, mqkey = %d, msgget[%m]\n", mqkey);
        exit(-1);
    }

    return 0;
}

//仅在整个spp实例退出时才删除消息队列
//所以此方法仅被controller进程调用
void CMQCommu::fini()
{
    if (mqid_ > 0)
    {
        msgctl(mqid_, IPC_RMID, NULL);
    }
}

int CMQCommu::recv(TProcMonMsg* msg, long msgtype)
{
    int ret = 0;

    do
    {
        ret = msgrcv(mqid_, (void*)msg, sizeof(TProcMonMsg) - sizeof(long), msgtype, IPC_NOWAIT);
    }
    while (ret == -1 && errno == EINTR);

    return ret;
}

int CMQCommu::send(TProcMonMsg* msg)
{
    int ret = 0;
    msg->timestamp_ = time(NULL);

    while ((ret = msgsnd(mqid_, (const void*)msg, msg->msglen_, IPC_NOWAIT) != 0) && (errno == EINTR));

    return ret;
}


/////////////////////////////////////////////////////////////////////////////////////////////
#define ADJUST_PROC_DELAY 15			//进程调整的延迟时间
#define ADJUST_PROC_CYCLE 1200			//进程调整周期
#define MIN_NOTIFY_TIME_CYCLE	30		//进程告警最小间隔时间

bool pid_isok(int pid)
{
    int result = kill(pid, 0);

    if (result == 0 || errno != ESRCH)
        return true;

    return false;

}

void print_monitor_info(MON_PROC_INFO* shmmem_, int max_proc = 0)
{

    int count = 0;
    int maxproc = shmmem_[0].id + 1;

    if (max_proc > 0)
        maxproc = max_proc;


    printf("print_monitor_info begin....\n");

    for (int i = 1; i < (maxproc); i++)
    {
        if ((shmmem_[i].groupid != -1) && (shmmem_[i].pid != -1))
        {
            count++;
            printf("\nshow monitor info[%d]:%s gpid %d pid %d curr_seq %d %d\n",
                   i, shmmem_[i].name, shmmem_[i].groupid,
                   shmmem_[i].pid, shmmem_[i].curr_seq, shmmem_[i].last_check_sep);
        }

        if (count > shmmem_[0].id)
            break;
    }

    printf("print_monitor_info end....\n");
}

CTProcMonSrv::CTProcMonSrv(): commu_(NULL)
{
    cur_group_ = 0;
    memset(proc_groups_, 0x0, sizeof(TProcGroupObj) * MAX_PROC_GROUP_NUM);

    for (int i = 0; i < MAX_PROC_GROUP_NUM; ++i)
        proc_groups_[i].curprocnum_ = GROUP_UNUSED;

    msg_[1].msglen_ = (long)(((TProcMonMsg*)0)->msgcontent_) + sizeof(TProcInfo) - sizeof(long);
    msg_[1].srctype_ = (MSG_VERSION << 1) | MSG_SRC_SERVER;

}

CTProcMonSrv::~CTProcMonSrv()
{
    TProcGroupObj* group;
    TProcObj* proc = NULL;
    TProcObj* prev = NULL;

    for (int i = 0; i < cur_group_; ++i)
    {
        group = &proc_groups_[i];

        if (group->curprocnum_ != GROUP_UNUSED)
        {
            for (int j = 0; j < BUCKET_SIZE; ++j)
            {
                prev = NULL;
                list_for_each_entry(proc, &group->bucket_[j], list_)
                {
                    if (likely(prev))
                    {
                        delete prev;
                    }

                    prev = proc;
                }

                if (likely(prev))
                    delete prev;
            }
        }
    }

    if (likely(commu_))
    {
        commu_->fini();
        delete commu_;
    }
}
int CTProcMonSrv::add_group(const TGroupInfo* groupinfo)
{
    if (cur_group_ > MAX_PROC_GROUP_NUM || cur_group_ != groupinfo->groupid_)
        return -1;

    TProcGroupObj* procgroup = &proc_groups_[cur_group_++];
    procgroup->curprocnum_ = 0;
    procgroup->errprocnum_ = 0;
    memcpy(&procgroup->groupinfo_, groupinfo, sizeof(TGroupInfo));

    for (int i = 0; i < BUCKET_SIZE; ++i)
        INIT_LIST_HEAD(&(procgroup->bucket_[i]));

    return 0;
}

int CTProcMonSrv::mod_group(int groupid, const TGroupInfo* groupinfo)
{
    //如果组id是新增的，则调用addgroup
    if (groupid == cur_group_) {
        add_group(groupinfo);
        return 0;
    }
    else if (groupid > cur_group_) {
        return -1;
    }

    TProcGroupObj* procgroup = &proc_groups_[groupid];
    memcpy(&procgroup->groupinfo_, groupinfo, sizeof(TGroupInfo));

    return 0;
}

void CTProcMonSrv::set_commu(CCommu* commu)
{
    if (commu_)
        delete commu_;

    commu_ = commu;

    do_recv(0);
}

void CTProcMonSrv::run()
{
    do_recv(0);
    do_check();
}

void CTProcMonSrv::query(TProcQueryObj*& result, int& num)
{
    if (cur_group_ <= 0)
        return;

    //调用者需要释放result分配的内存空间
    num = cur_group_;
    result = new TProcQueryObj[num];
    memset(result, 0x0, sizeof(TProcQueryObj) * num);
    TProcGroupObj* group = NULL;
    TProcObj* proc = NULL;
    TProcQueryObj* qobj = NULL;

    for (int i = 0; i < num; ++i)
    {
        qobj = &result[i];
        group = &proc_groups_[i];

        if (group->curprocnum_ != GROUP_UNUSED)
        {
            qobj->group = group;
            qobj->proc = new TProcObj*[group->curprocnum_ + 1];
            int count = 0;

            for (int j = 0; j < BUCKET_SIZE; ++j)
            {
                list_for_each_entry(proc, &group->bucket_[j], list_)
                {
                    if (count < group->curprocnum_)
                        qobj->proc[count++] = proc;
                    else
                        break;
                }
            }

            if (count <= group->curprocnum_)		//最后一个NULL作为结束标志
                qobj->proc[count] = NULL;
        }
        else
        {
            qobj->group = NULL;
            qobj->proc = NULL;
        }
    }
}


/**
 * @brief 等待segv进程退出
 */
void CTProcMonSrv::segv_wait(TProcInfo *procinfo)
{
    const int32_t kill_cnt = 100;
    int32_t       proc_no  = procinfo->procid_;

    for (int32_t i = 0; i < kill_cnt; i++)
    {
        if ((kill(proc_no, 0) == -1) && errno == ESRCH)
        {
            DO_LOG_PROCMON(LOG_FATAL, "pid[%d] is dead, quit success, %d.\n", proc_no, i);
            return;
        }

        usleep(1000);
    }

    DO_LOG_PROCMON(LOG_FATAL, "pid[%d] is dead, [D/T] exception.\n", proc_no);

    return;
}


void CTProcMonSrv::process_exception(void)
{
    TProcInfo* procinfo = (TProcInfo*)(msg_[0].msgcontent_);
    TProcGroupObj* group = &proc_groups_[procinfo->groupid_];
    TProcObj *proc = NULL;
    int event = 0;

    if(group->curprocnum_ == GROUP_UNUSED)
    {
        DO_LOG_PROCMON(LOG_FATAL, "group[%d] pid[%d] is dead, exception signal.\n", procinfo->groupid_, procinfo->procid_);
        return;
    }
    
    DO_LOG_PROCMON(LOG_FATAL, "group[%d] pid[%d] is dead, exception signal[%d].\n", 
                   procinfo->groupid_, procinfo->procid_, EXCEPTION_TYPE(msg_[0].srctype_));

    segv_wait(procinfo);

    proc = find_proc(procinfo->groupid_, procinfo->procid_);
    if ( NULL != proc)
    {
        proc->status_ = PROCMON_STATUS_OK;
        group->groupinfo_.adjust_proc_time = time(NULL);
        event |= PROCMON_EVENT_PROCDEAD;
        do_event(event, NULL, (void*)proc);
    }

    return;
}

bool CTProcMonSrv::do_recv(long msgtype)
{
    int ret = 0;
    TProcInfo* procinfo = NULL;
    TProcObj* proc = NULL;
    map<int, map<int, int > >::iterator it;

    do
    {
        ret = commu_->recv(&msg_[0], msgtype);

        if (unlikely(ret > 0))
        {
            if ((msg_[0].srctype_ & 0x1) == MSG_SRC_CLIENT)   //from client
            {
            	//秒起处理
            	if (EXCEPTION_TYPE(msg_[0].srctype_))
				{
					process_exception();
					continue;
				}

                procinfo = (TProcInfo*)(msg_[0].msgcontent_);

			    TProcGroupObj* group = find_group(procinfo->groupid_);
				if (NULL == group)
				{
					//不存在该组信息，忽略这条消息					
					continue;
				}
                
                proc = find_proc(procinfo->groupid_, procinfo->procid_);

                // 非热重启处理
                if (group->groupinfo_.reload_ == PROC_RELOAD_NORMAL)
                {
                    if (proc)
                    {
                        memcpy(&proc->procinfo_, procinfo, sizeof(TProcInfo));                  
                    }
                    else
                    {
                        add_proc(procinfo->groupid_, procinfo);
                    }

                    continue;
                }

                // 热重启处理
				TGroupInfo* reload_group = &group->groupinfo_;
                if (proc)  // 热重启已经处理过的进程
                {
                    memcpy(&proc->procinfo_, procinfo, sizeof(TProcInfo));                  
                }
                else if (procinfo->reserve_[0] == 1) // 热重启新进程 spp_handle_init 尚未结束，忽略该信号
                {
                    DO_LOG_PROCMON(LOG_DEBUG, "group[%d] pid[%d] ready call spp_handle_init.\n", 
                                   procinfo->groupid_, procinfo->procid_);
                }
                else if(!check_reload_old_proc(reload_group->groupid_, procinfo->procid_))
                {
                    add_proc(procinfo->groupid_, procinfo);
                }
            }
        }
    }
    while (ret > 0);

    return true;
}

void CTProcMonSrv::reload_kill_group(int groupid, int signo)
{
    std::map<int, std::map<int,int> >::iterator it;

    it = reload_tag.find(groupid);
    if (it == reload_tag.end())
    {
        return;
    }

    std::map<int,int> &procmap = it->second;
    std::map<int,int>::iterator proc_it;
    for (proc_it = procmap.begin(); proc_it != procmap.end(); proc_it++)
    {
        int procid = proc_it->second;

        if ((kill(procid, 0) == -1) && errno == ESRCH) {
            continue;
        }

        do_kill(procid, signo);
    }

    return;
}

// 检查热重启老进程是否都退出,如果都退出了，清掉老进程数据
bool CTProcMonSrv::reload_check_group_dead(int groupid)
{
    std::map<int, std::map<int,int> >::iterator it;
    it = reload_tag.find(groupid);
    if (it == reload_tag.end())
    {
        return true;
    }

    std::map<int,int> &procmap = it->second;
    std::map<int,int>::iterator proc_it;

    for (proc_it = procmap.begin(); proc_it != procmap.end(); proc_it++)
    {
        int procid = proc_it->second;

        if ((kill(procid, 0) == -1) && errno == ESRCH) {
            continue;
        }

        return false;
    }

    reload_tag.erase(it);

    return true;
}

bool CTProcMonSrv::check_groupbusy(int groupid)
{
    TProcGroupObj* groupobj = &proc_groups_[groupid];
    TGroupInfo* group = &(groupobj->groupinfo_);

    if ((Q_STATINFO*)group->q_recv_pstat == NULL)
    {
        if (group->groupid_ == 0)
        {
            return false;
        }

        key_t key = pwdtok(group->groupid_ * 2);

        void* pinfo = NULL;
        int shmid_ = -1;

        if ((shmid_ = shmget(key, 0, 0)) == -1)
        {
            return false;
        }

        if (shmid_ < 0)
        {
            return false;
        }

        pinfo = (void*) shmat(shmid_, NULL, 0);

        if (likely(pinfo != MAP_FAILED))
        {
        }
        else
        {
            return false;
        }

        group->q_recv_pstat = pinfo;
    }

    Q_STATINFO* q_recv_pstat = (Q_STATINFO*)group->q_recv_pstat;
    int msg_count;
    msg_count = atomic_read(&(q_recv_pstat->msg_count));

    if (msg_count <= 0)
    {
        atomic_set(&q_recv_pstat->process_count, 0);
        return false;
    }

    int proc_count = atomic_read(&(q_recv_pstat->process_count));

    atomic_set(&q_recv_pstat->process_count, 0);

    if (proc_count < msg_count)
    {
        return true;
    }

    return false;

}

bool CTProcMonSrv::do_check()
{		
    TProcObj* proc = NULL;
    TProcGroupObj* group = NULL;
    int i, j;

    for (i = 0; i < cur_group_; ++i)
    {
        group = &proc_groups_[i];

        if (group->curprocnum_ != GROUP_UNUSED)
        {
            // 首先检查进程组是否需要reload
            TGroupInfo* reload_group = &group->groupinfo_;
            if(reload_group->reload_ > 0)
            {  // 热重启处理逻辑
                switch (reload_group->reload_)
                {
                    case PROC_RELOAD_START:
                    {
						DO_LOG_PROCMON(LOG_DEBUG, "reload state(PROC_RELOAD_START), fork %u processes\n", reload_group->minprocnum_);
                        do_fork(reload_group->basepath_, reload_group->exefile_, reload_group->etcfile_, reload_group->minprocnum_, reload_group->groupid_, reload_group->affinity_);
                        reload_group->reload_ = PROC_RELOAD_WAIT_NEW;
                        reload_group->reload_time = time(NULL);
                        break;
                    }

                    case PROC_RELOAD_WAIT_NEW:
                    {
                        // 启动进程最长等待时间 30 秒
						DO_LOG_PROCMON(LOG_DEBUG, "reload state(PROC_RELOAD_WAIT_NEW)\n");
                        if (((unsigned)group->curprocnum_ >= reload_group->minprocnum_) || (reload_group->reload_time + 30 < time(NULL)))
                        {
                            reload_kill_group(reload_group->groupid_, SIGUSR2);
                            reload_group->reload_ = PROC_RELOAD_KILLOLD;
                            reload_group->reload_time = time(NULL);
                        }
                        break;
                    }

                    case PROC_RELOAD_KILLOLD:
                    {
                        // 杀死老进程
						DO_LOG_PROCMON(LOG_DEBUG, "reload state(PROC_RELOAD_KILLOLD)\n");
                        reload_kill_group(reload_group->groupid_, SIGUSR2);
                        reload_group->reload_ = PROC_RELOAD_CLEAR;
                        reload_group->reload_time = time(NULL);
                        break;
                    }

                    case PROC_RELOAD_CLEAR:
                    {
                        // 检查老进程是否都退出
						DO_LOG_PROCMON(LOG_DEBUG, "reload state(PROC_RELOAD_CLEAR)\n");
                        if (reload_check_group_dead(reload_group->groupid_))
                        {
                            reload_group->reload_ = PROC_RELOAD_NORMAL;
                            reload_group->reload_time = time(NULL);
                            break;
                        }

                        // 30秒没有退出，直接杀死
                        if (reload_group->reload_time + 30 < time(NULL))
                        {
                            reload_kill_group(reload_group->groupid_, SIGKILL);
                            break;
                        }

                        break;
                    }

                    default:
                    {
						DO_LOG_PROCMON(LOG_DEBUG, "invalid reload state(%d)\n", reload_group->reload_);
                        break;
                    }
                }

            }
            else
            { // 非热重启
                check_group(&group->groupinfo_, group->curprocnum_);
                for (j = 0; j < BUCKET_SIZE; ++j)
                {
                    list_for_each_entry(proc, &group->bucket_[j], list_)
                    {
                        if (unlikely(check_proc(&group->groupinfo_, &proc->procinfo_)))
                        {
                            //some proc has been deleted, go back to list head & travel again
                            --j;
                            break;
                        }
                    }
                }
            }
        }
    }

    return true;
}

int CTProcMonSrv::dump_pid_list(char* buf, int len)
{
#define WRITE_BUF(fmt, args...) \
    do{ \
        used += snprintf(buf + used, len - used, fmt, ##args); \
    }while(0)

    TProcObj* proc = NULL;
    TProcGroupObj* group = NULL;
    static int mypid = getpid();
    int used = 0;


    WRITE_BUF("%d [SRPC_CTRL]\n", mypid); // 第一行输出PID

    for (int i = 0; i < cur_group_; ++i)
    {
        group = &proc_groups_[i];
        if (group->curprocnum_ == GROUP_UNUSED)
            continue;

        WRITE_BUF("\nGROUPID[%d]\n", i);
        for (int j = 0; j < BUCKET_SIZE; ++j)
        {
            list_for_each_entry(proc, &group->bucket_[j], list_)
            {
                WRITE_BUF("%d\n", proc->procinfo_.procid_);
            }
        }
    }

    return used;
}


void CTProcMonSrv::killall(int signo)
{
    TProcObj* proc = NULL;
    TProcGroupObj* group = NULL;
    int i, j;

    for (i = 0; i < cur_group_; ++i)
    {
        group = &proc_groups_[i];

        if (group->curprocnum_ != GROUP_UNUSED)
        {
            for (j = 0; j < BUCKET_SIZE; ++j)
            {
                list_for_each_entry(proc, &group->bucket_[j], list_)
                {
                    do_kill(proc->procinfo_.procid_, signo);
                }
            }
        }
    }
}
int CTProcMonSrv::add_proc(int groupid, const TProcInfo* procinfo)
{
    if (groupid >= cur_group_)
    {
        return -1;
    }


    TProcObj* proc = new TProcObj;


    memcpy(&proc->procinfo_, procinfo, sizeof(TProcInfo));
    proc->status_ = PROCMON_STATUS_OK;
    INIT_LIST_HEAD(&proc->list_);

    TProcGroupObj* group = &proc_groups_[groupid];
    list_add(&proc->list_, &group->bucket_[procinfo->procid_ % BUCKET_SIZE]);
    group->curprocnum_++;

    return 0;
}

TProcGroupObj* CTProcMonSrv::find_group(int groupid)
{
    TProcGroupObj* group = NULL;
    int i;

    for (i = 0; i < cur_group_; ++i)
    {
        group = &proc_groups_[i];

        if (group->groupinfo_.groupid_ == groupid && group->curprocnum_ != GROUP_UNUSED)
        {
        	return group;
        }
    }

	return NULL;
}

TProcObj* CTProcMonSrv::find_proc(int groupid, int procid)
{
    if (groupid >= cur_group_)
        return NULL;

    TProcGroupObj* group = &proc_groups_[groupid];
    int bucket = procid % BUCKET_SIZE;
    TProcObj* proc = NULL;
    list_for_each_entry(proc, &group->bucket_[bucket], list_)
    {
        if (proc->procinfo_.procid_ == procid)
            return proc;
    }
    return NULL;
}
void CTProcMonSrv::del_proc(int groupid, int procid)
{
    if (groupid >= cur_group_)
        return;

    TProcGroupObj* group = &proc_groups_[groupid];
    int bucket = procid % BUCKET_SIZE;
    TProcObj* proc = NULL;
    list_for_each_entry(proc, &group->bucket_[bucket], list_)
    {
        if (proc->procinfo_.procid_ == procid)
        {
            list_del(&proc->list_);
            delete proc;
            break;
        }
    }
    group->curprocnum_--;
}

bool CTProcMonSrv::check_reload_old_proc(int groupid, int procid)
{
    std::map<int, std::map<int, int> >::iterator it;

    it = reload_tag.find(groupid);
    if (it == reload_tag.end())
    {
        return false;
    }

    std::map<int, int> &procmap = it->second;
    std::map<int, int>::iterator proc_it = procmap.find(procid);
    if (proc_it == procmap.end())
    {
        return false;
    }

    return true;
}

void CTProcMonSrv::pre_reload_group(int groupid, const TGroupInfo *groupinfo)
{
    TProcGroupObj* procgroup = &proc_groups_[groupid];
    TProcObj* procobj;
    TProcObj* tmp;
    std::map<int, int> procmap;

    for (int i = 0; i < BUCKET_SIZE; i++)
    {
        list_for_each_entry_safe(procobj, tmp, &procgroup->bucket_[i], list_)
        {
            int proc_id = procobj->procinfo_.procid_;
            procmap[proc_id] = proc_id;
            del_proc(groupid, proc_id);
        }
    }

    // 保存老进程
    reload_tag[groupid] = procmap;

    mod_group(groupid, groupinfo);
    procgroup->curprocnum_ = 0;
    procgroup->errprocnum_ = 0;
}

void CTProcMonSrv::check_group(TGroupInfo* group, int curprocnum)
{
    int event = 0;
    int procdiff = 0;
    time_t now = time(NULL);

    if (group->adjust_proc_time + ADJUST_PROC_DELAY > now)
    {
        return;
    }

    if (unlikely((procdiff = (int)group->minprocnum_ - curprocnum) > 0))
    {
        DO_LOG_PROCMON(LOG_FATAL, "group[%d] current proc#[%d], fork [%d] processes.\n",
                group->groupid_, curprocnum, procdiff);
        event |= PROCMON_EVENT_PROCDOWN;
    }
    else if (unlikely((procdiff = curprocnum - (int)group->maxprocnum_) > 0))
    {
        DO_LOG_PROCMON(LOG_FATAL, "group[%d] current proc#[%d], kill [%d] processes.\n",
                group->groupid_, curprocnum, procdiff);
        event |= PROCMON_EVENT_PROCUP;
    }
    else
    {
        if (!check_groupbusy(group->groupid_))
        {
            if (((int)group->minprocnum_ < curprocnum))
            {
                DO_LOG_PROCMON(LOG_FATAL, "group[%d] is not busy, kill one process. current proc#[%d]\n",
                        group->groupid_, curprocnum);
                event |= PROCMON_EVENT_PROCUP;
                procdiff = 1;
            }
        }
        else
        {
            if (((int)group->maxprocnum_ > curprocnum))
            {
                DO_LOG_PROCMON(LOG_FATAL, "group[%d] is busy, fork one process. current proc#[%d]\n",
                        group->groupid_, curprocnum);
                event |= PROCMON_EVENT_PROCDOWN;
                procdiff = 1;
            }
        }
    }

    if (unlikely(event != 0))
    {
        group->adjust_proc_time = now;
        do_event(event, (void*)&procdiff, (void*)group);
    }
}
bool CTProcMonSrv::check_proc(TGroupInfo* group, TProcInfo* proc)
{
    int event = 0;
    time_t now = time(NULL);

    if (unlikely(proc->timestamp_ < now - (time_t)group->heartbeat_))
    {
        DO_LOG_PROCMON(LOG_FATAL, "group[%d] pid[%d] is dead, no heartbeat.\n",
                group->groupid_, proc->procid_);
        event |= PROCMON_EVENT_PROCDEAD;
    }

    ((TProcObj*)proc)->status_ = PROCMON_STATUS_OK;

    if (unlikely(event != 0))
    {
        group->adjust_proc_time = now;
        return do_event(event, (void*)&proc->procid_, (void*)proc);
    }
    else
        return false;
}
bool CTProcMonSrv::do_event(int event, void* arg1, void* arg2)
{
    //////////////////////group event//////////////////////////
    if (event & PROCMON_EVENT_PROCDOWN)
    {
        int diff = *((int*)arg1);
        TGroupInfo* group = (TGroupInfo*)arg2;
        if(group->groupid_ != 0)
        {
            MONITOR_INC(MONITOR_WORKER_LESS, diff);
        }
        else
        {
            MONITOR_INC(MONITOR_PROXY_LESS, diff);
        } 
        do_fork(group->basepath_, group->exefile_, group->etcfile_, diff, group->groupid_, group->affinity_);
        return false;
    }

    if (event & PROCMON_EVENT_PROCUP)
    {
        TProcGroupObj* group = (TProcGroupObj*)arg2;
        int diff = *((int*)arg1);
        int groupid = group->groupinfo_.groupid_;
        TProcObj* proc = NULL;
        int procid = 0;
        int count = 0;

        for (int i = 0; i < BUCKET_SIZE; ++i)
        {
            list_for_each_entry(proc, &group->bucket_[i], list_)
            {
                procid = proc->procinfo_.procid_;
                do_kill(procid, SIGUSR1);
                do_recv(procid);
                del_proc(groupid, procid);
                if(groupid != 0)
                {
                    MONITOR(MONITOR_WORKER_MORE);
                }
                else
                {
                    MONITOR(MONITOR_PROXY_MORE);
                }
                count++;
                i--;
                break;
            }

            if (count >= diff)
                break;
        }

        return false;
    }

    /////////////////////////proc event/////////////////////////
    TProcInfo* proc = (TProcInfo*)arg2;
    ((TProcObj*)proc)->status_ = PROCMON_STATUS_OK;

    if (event & PROCMON_EVENT_PROCDEAD)
    {
        TGroupInfo* group = &proc_groups_[proc->groupid_].groupinfo_;
        int signalno = group->exitsignal_;
        do_kill(proc->procid_, signalno); // 默认为SIGNAL[9]，见CDefaultCtrl::initconf
        usleep(10000);
        ((TProcGroupObj *) group)->errprocnum_++;
        if (kill(proc->procid_, 0) == -1 && errno == ESRCH) //确信已经kill掉进程了才将fork新的进程.
        {
            do_recv(proc->procid_); //del msg in mq
            del_proc(proc->groupid_, proc->procid_);        //del from proc list
            do_fork(group->basepath_, group->exefile_, group->etcfile_, 1, group->groupid_, group->affinity_);     //fork a process
            if(group->groupid_ == 0)
            {
                MONITOR(MONITOR_PROXY_DOWN);
            }
            else
            {
                MONITOR(MONITOR_WORKER_DOWN);
            } 
            return true;            //change the proc list
        }
        else
        {
            DO_LOG_PROCMON(LOG_FATAL, "The maybe D/T status process(%u) is finded\n", proc->procid_);
            if(group->groupid_ == 0)
            {
                MONITOR(MONITOR_PROXY_DT);
            }
            else
            {
                MONITOR(MONITOR_WORKER_DT);
            }
            return false;           //not change the proc list
        }

    }

    return false;
}

void CTProcMonSrv::do_fork(const char* basepath, const char* exefile, const char* etcfile, int num, int groupid, unsigned mask)
{
    char cmd_buf[256] = {0};
    snprintf(cmd_buf, sizeof(cmd_buf) - 1, "%s/%s %s/%s", basepath, exefile, basepath, etcfile);

    if (mask > 0)
        set_affinity(mask); // 让子进程继承

    for (int i = 0; i < num; ++i)
    {
        system(cmd_buf);
        usleep(12000); // 控制速度，预防框架/插件实现上竞争引发的bug
    }
}

int CTProcMonSrv::set_affinity(const uint64_t mask)
{
    unsigned long mask_use = (unsigned long)mask;
    if (sched_setaffinity(getpid(), (uint32_t)sizeof(mask_use), (cpu_set_t *)&mask_use) < 0)
    {
        return -1;
    }

    return 0;
}

void CTProcMonSrv::do_kill(int procid, int signo)
{
    kill(procid, signo);
}

void CTProcMonSrv::do_order(int groupid, int procid, int eventno, int cmd, int arg1, int arg2)
{
    msg_[1].msgtype_ = procid;
    TProcEvent* event = (TProcEvent*)msg_[1].msgcontent_;
    event->groupid_ = groupid;
    event->procid_ = procid;
    event->cmd_ = cmd;
    event->arg1_ = arg1;
    event->arg2_ = arg2;
    commu_->send(&msg_[1]);
}

void CTProcMonSrv::set_tlog(CTLog* log)
{
    log_ = log;
}

/////////////////////////////////////////////////////////////////////////////////////
CTProcMonCli::CTProcMonCli(): commu_(NULL)
{
    //msg_[0].msgtype_ = MSG_ID_SERVER;
    msg_[0].msgtype_ = getpid();

    //发送的长度为总长度减去消息队列的type字段长度.
    msg_[0].msglen_ = (long)(((TProcMonMsg*)0)->msgcontent_) + sizeof(TProcInfo) - sizeof(long);
    msg_[0].srctype_ = (MSG_VERSION << 1) | MSG_SRC_CLIENT;
}
CTProcMonCli::~CTProcMonCli()
{
    if (commu_)
        delete commu_;
}
void CTProcMonCli::set_commu(CCommu* commu)
{
    if (commu_)
        delete commu_;

    commu_ = commu;
}

void CTProcMonCli::run()
{
    //send to server
    commu_->send(&msg_[0]);
}

