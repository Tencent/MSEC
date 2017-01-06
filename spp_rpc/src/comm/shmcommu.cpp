
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


#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "tshmcommu.h"
#include "misc.h"
#include "atomic.h"
#include <stdlib.h>
#include "sys/uio.h"
#include "hide_private_tp.h"
#include "hex_dump.h"

#include "global.h"
#include "comm_def.h"
#include "frame.h"
#include "tbase/tshmcommu.h"
#include "shmcommu.h"

using namespace tbase::tcommu;
using namespace spp::comm;


/////////////////////////////////////////////////////////////////////////////////////////////


void *getmemory(int shmkey, int shmsize)
{
    int32_t shmid;
    void *shmmem;

    if ((shmid = shmget(shmkey, shmsize, 0)) != -1)
    {
        shmmem = shmat(shmid, NULL, 0);

        if (likely(shmmem != MAP_FAILED))
        {
            return shmmem;
        }

        fprintf( stderr, "Shm map failed:shmkey=%x,shmsize=%d.ErrMsg:%m\n", shmkey, shmsize);
        return NULL;
    }
    else
    {
        shmid = shmget(shmkey, shmsize, IPC_CREAT | 0666);

        if (shmid != -1)
        {
            shmmem = shmat(shmid, NULL, 0);

            if (shmmem != MAP_FAILED)
            {
                return shmmem;
            }
            else
            {
                fprintf( stderr, "Shm map failed:shmkey=%x,shmsize=%d.ErrMsg:%m\n", shmkey, shmsize);
                return NULL;
            }
        }
        else
        {
            fprintf( stderr, "Shm create failed:shmkey=%x,shmsize=%d.ErrMsg:%m\n", shmkey, shmsize);
            return NULL;
        }
    }

    return NULL;
}



CShmCommu::CShmCommu(): locktype_(0)
{
    buff_blob_.len = 0;
    buff_blob_.data = NULL;
    buff_blob_.owner = NULL;
    buff_blob_.extdata = NULL;
}
CShmCommu::~CShmCommu()
{
    fini();
}
void CShmCommu::fini()
{
    if (buff_blob_.data != NULL)
    {
        free((char *)buff_blob_.data - 4);
        buff_blob_.data = NULL;
    }
}

int CShmCommu::init(const void* config)
{
    int ret = 0;
    void *mem;

    fini();
    TShmCommuConf* conf = (TShmCommuConf*)config;

    locktype_  = conf->locktype_;
    notify_fd_ = conf->notifyfd_;

    mem = getmemory(conf->shmkey_producer_, conf->shmsize_producer_);
    if (NULL == mem) {
        return -1;
    }

    ret = mem_queue_init(&producer_, mem, conf->shmsize_producer_, 4096);
    if (ret < 0) {
        return -2;
    }

    mem = getmemory(conf->shmkey_comsumer_, conf->shmsize_comsumer_);
    if (NULL == mem) {
        return -3;
    }

    ret = mem_queue_init(&comsumer_, mem, conf->shmsize_comsumer_, 4096);
    if (ret < 0) {
        return -4;
    }

    //初始化其他变量
    buff_size_ = conf->shmsize_comsumer_;	//最大不会超过comsumer使用的共享内存大小
    buff_blob_.len = buff_size_;
    buff_blob_.data = (char *)malloc(sizeof(char) * buff_size_) + 4;
    buff_blob_.owner = this;
    buff_blob_.extdata = NULL;
    //buff_blob_.extdata = malloc( sizeof(TConnExtInfo) );

    return 0;
}

int CShmCommu::clear()
{
    return 0;
}

int CShmCommu::poll(bool block)
{
    int processed = 0;
    unsigned flow = 0;

    do
    {
        int ret = mem_queue_pop_nm(&comsumer_, (char *)buff_blob_.data - 4, (int)buff_size_);
        if (ret < (int)sizeof(flow))
        {
            break; // when empty or error
        }
        ++processed;

        buff_blob_.len = ret - 4;
        flow = *(unsigned*)((char *)buff_blob_.data - 4);

        func_list_[CB_RECVDATA](flow, &buff_blob_, func_args_[CB_RECVDATA]);
    }
    while (block);

    return processed;
}

int CShmCommu::sendto(unsigned flow, void* arg1, void* arg2)
{
    int ret = 0;
    struct iovec iov[2];
    blob_type* blob = (blob_type*)arg1;

    iov[0].iov_base = &flow;
    iov[0].iov_len  = sizeof(flow);
    iov[1].iov_base = blob->data;
    iov[1].iov_len  = blob->len;

    ret = mem_queue_pushv(&producer_, iov, 2);
    INTERNAL_LOG->LOG_P_FILE(LOG_DEBUG, "sendto return [%d]\n", ret);

    if (ret == 1) {
        write(notify_fd_, "!", 1);
    }

    if ( likely( ret >= 0 ) )
    {
        if (func_list_[CB_SENDDATA] != NULL)
        {
            func_list_[CB_SENDDATA](flow, blob, func_args_[CB_SENDDATA]);
        }
        ret = 0; // 兼容性
    }
    else
    {
        if (func_list_[CB_SENDERROR] != NULL)
        {
            func_list_[CB_SENDERROR](flow, blob, func_args_[CB_SENDERROR]);
        }

        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "sendto error, return [%d]\n", ret);
    }

    return ret;
}

int CShmCommu::ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2)
{
#if 0
    switch (type)
    {
    case CT_STAT:
    {
        char* buf = (char*)arg1;
        int* len = (int*)arg2;
        time_t now = time(NULL);
        struct tm tmm;
        localtime_r(&now, &tmm);
        //buf size 1<<16 refer to char ctor_stat[] size
        *len += snprintf(buf + *len, (1 << 16) - *len - 1, "TShmCommu[%-5d],%04d-%02d-%02d %02d:%02d:%02d\n", (int)syscall(__NR_gettid),
                         tmm.tm_year + 1900, tmm.tm_mon + 1, tmm.tm_mday, tmm.tm_hour, tmm.tm_min, tmm.tm_sec);
        *len += snprintf(buf + *len, (1 << 16) - *len - 1, "%-20s%-10s|%-10s|%-10s|%-10s|%-10s|%-10s\n", "Type", "ShmKey", "ShmID",
                         "ShmSize", "Total", "Used", "Free");

        producer_->getstat(mq);
        *len += snprintf(buf + *len, (1 << 16) - *len - 1, "%-20s%-10x|%-10d|%-10d|%-10d|%-10d|%-10d\n", "Producer", mq.shmkey_,
                         mq.shmid_, mq.shmsize_, mq.totallen_, mq.usedlen_, mq.freelen_);

        comsumer_->getstat(mq);
        *len += snprintf(buf + *len, (1 << 16) - *len - 1, "%-20s%-10x|%-10d|%-10d|%-10d|%-10d|%-10d\n", "Comsumer", mq.shmkey_,
                         mq.shmid_, mq.shmsize_, mq.totallen_, mq.usedlen_, mq.freelen_);
        break;
    }
    case CT_DISCONNECT:
    case CT_CLOSE:
        break;
    }
#endif

    return 0;
}

int CShmCommu::get_msg_count()
{
    /*
    TMQStat mq;
    comsumer_->getstat(mq);
    return mq.totallen_;*/
    return 0;
}

