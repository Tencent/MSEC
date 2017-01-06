
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
#include "hide_private_tp.h"
#include "hex_dump.h"

#include "../global.h"
#include "../comm_def.h"
#include "../frame.h"

#if !__GLIBC_PREREQ(2, 3)
#define __builtin_expect(x, expected_value) (x)
#endif
#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

using namespace spp::comm;
using namespace spp::global;
using namespace spp::statdef;

#define C_PUB_HEAD_SIZE  8
#define C_HEAD_SIZE  C_PUB_HEAD_SIZE
#define C_SAFE_AREA_LEN  4
#define C_SAFE_AREA_VAL  0x58505053 //"SPPX"

using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;

CShmMQ::CShmMQ(): shmkey_(0), shmsize_(0), shmid_(0), 
    shmmem_(NULL), pstat_(NULL), head_(NULL), tail_(NULL), block_(NULL), blocksize_(0)
{
    msg_timeout = 0;
}

CShmMQ::~CShmMQ()
{
    fini();
}

int CShmMQ::getmemory(int shmkey, int shmsize)
{
    shmsize += sizeof(Q_STATINFO);

    if ((shmid_ = shmget(shmkey, shmsize, 0)) != -1)
    {
        shmmem_ = shmat(shmid_, NULL, 0);

        if (likely(shmmem_ != MAP_FAILED))
        {
            pstat_ = (Q_STATINFO*)shmmem_;
            shmmem_ = (void*)((unsigned long)shmmem_ + sizeof(Q_STATINFO));
            return 0;
        }
        else
        {
            fprintf( stderr, "Shm map failed:shmkey=%x,shmsize=%d.ErrMsg:%m\n", shmkey_, shmsize_);
            return COMMU_ERR_SHMMAP;
        }
    }
    else
    {
        shmid_ = shmget(shmkey, shmsize, IPC_CREAT | 0666);

        if (likely(shmid_ != -1))
        {
            shmmem_ = shmat(shmid_, NULL, 0);

            if (likely(shmmem_ != MAP_FAILED))
            {
                pstat_ = (Q_STATINFO*)shmmem_;
                shmmem_ = (void*)((unsigned long)shmmem_ + sizeof(Q_STATINFO));
                return 1;
            }
            else
            {
                fprintf( stderr, "Shm map failed:shmkey=%x,shmsize=%d.ErrMsg:%m\n", shmkey_, shmsize_);
                return COMMU_ERR_SHMMAP;
            }
        }
        else
        {
            fprintf( stderr, "Shm create failed:shmkey=%x,shmsize=%d.ErrMsg:%m\n", shmkey_, shmsize_);
            return COMMU_ERR_SHMNEW;
        }
    }
}

void CShmMQ::fini()
{
    if (pstat_) // pstat_就是地址开始
        shmdt((const void*)pstat_);
}

int CShmMQ::init(int shmkey, int shmsize)
{
    fini();

    shmkey_ = shmkey;
    shmsize_ = shmsize;
    int ret = 0;

    /*
    	ret>0:create new shm segment
    	ret=0:use shm segment of the same key
    	ret<0:error occurs
    */
    if ((ret = getmemory(shmkey_, shmsize_)) > 0)
    {
        memset(shmmem_, 0x0, sizeof(unsigned) * 2);
    }
    else if (ret < 0)
    {
        switch (ret)
        {
        case COMMU_ERR_SHMMAP:
            break;
        case COMMU_ERR_SHMNEW:
            break;
        default:
            fprintf( stderr, "CShmMQ::getmomory() return %d, can not be recognised\n", ret);
        }

        return ret;
    }

    //the head of data section
    head_ = (unsigned*)shmmem_;
    //the tail of data section
    tail_ = head_ + 1;
    // pid field
    pid_ = (pid_t *)(tail_ + 1);
    //data section base address
    block_ = (char*)(pid_ + 1);
    //data section length
    blocksize_ = shmsize_ - sizeof(unsigned) * 2 - sizeof(pid_t);

    *pid_ = 0;

    return 0;
}

int CShmMQ::clear()
{
    if (!shmmem_)
    {
        fprintf( stderr, "shmmem_ is NULL.\n");
        return COMMU_ERR_SHM_NULL;
    }

    //clear head and tail of shm queue
    *pid_ = getpid();
    *head_ = 0;
    *tail_ = 0;
    *pid_ = 0;

    pstat_->msg_count = 0;

    return 0;
}

bool CShmMQ::do_check(unsigned head, unsigned tail)
{

    if ( likely( head <= blocksize_ && tail <= blocksize_ ) )
    {
        // head & tail 合法
        return true;
    }

    bool clr_flag = false;
    if ( *pid_ == 0 )
    {
        // need clear
        clr_flag = true;
    }
    else
    {
        if ( 0 == CMisc::check_process_exist(*pid_) )
        {
            // process of do clearing not exist, need clear
            clr_flag = true;
        }

        // other process is doing clearing
    }

    if ( clr_flag )
    {
        clear();
    }

    return false;

}


int CShmMQ::enqueue(const void* data, unsigned data_len, unsigned flow)
{
    unsigned head = *head_;
    unsigned tail = *tail_;

    // check if head & tail is valid
    if ( unlikely( !do_check(head, tail) ) )
    {
        return COMMU_ERR_CHKFAIL;
    }

    unsigned free_len = head > tail ? head - tail : head + blocksize_ - tail;
    unsigned tail_len = blocksize_ - tail;

    char sHead[C_HEAD_SIZE + C_SAFE_AREA_LEN] = {0};
    char *psHead = sHead;
    unsigned total_len = data_len + C_HEAD_SIZE + 2 * C_SAFE_AREA_LEN;

    //as to 4 possible queue free_len,enqueue data in 4 ways
    //  first, if no enough space?
    if (unlikely(free_len <= total_len))
    {
        if (!spp_global::mem_full_warning_script.empty())
            system(spp_global::mem_full_warning_script.c_str());

        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "Queue is full, free len:%u < blob len:%u\n", free_len, total_len);
        return COMMU_ERR_MQFULL;
    }

    *(unsigned *)psHead = C_SAFE_AREA_VAL;//set magic number "SPPX"
    memcpy(psHead + C_SAFE_AREA_LEN, &total_len, sizeof(unsigned));
    memcpy(psHead + C_SAFE_AREA_LEN + sizeof(unsigned), &flow, sizeof(unsigned));


    //  second, if tail space > 4+8+len+4
    //  copy 8 byte, copy data
    if (tail_len >= total_len)
    {
        //append head
        memcpy(block_ + tail, psHead, C_SAFE_AREA_LEN + C_HEAD_SIZE);
        //append data
        memcpy(block_ + tail + C_SAFE_AREA_LEN + C_HEAD_SIZE , data, data_len);
        //append C_SAFE_AREA_VAL
        *(unsigned*)(block_ + tail + C_SAFE_AREA_LEN + C_HEAD_SIZE + data_len) = C_SAFE_AREA_VAL;
        //recalculate tail_ position
        *tail_ += (data_len + C_HEAD_SIZE + C_SAFE_AREA_LEN * 2);
    }
    //  third, if tail space > 4+8 && < 4+8+len+4
    else if (tail_len >= C_HEAD_SIZE + C_SAFE_AREA_LEN && tail_len < C_HEAD_SIZE + 2 * C_SAFE_AREA_LEN + data_len)
    {
        //append head, 4+8 byte
        memcpy(block_ + tail, psHead, C_SAFE_AREA_LEN + C_HEAD_SIZE);

        //separate data into two parts
        unsigned first_len = tail_len - C_SAFE_AREA_LEN - C_HEAD_SIZE;
        unsigned second_len = data_len + C_SAFE_AREA_LEN - first_len;

        if (second_len >= C_SAFE_AREA_LEN)
        {
            //append first part of data, tail-4-8
            memcpy(block_ + tail + C_SAFE_AREA_LEN + C_HEAD_SIZE, data, first_len);
            //append left, second part of data
            memcpy(block_, ((char*)data) + first_len, second_len - C_SAFE_AREA_LEN);
            *(unsigned *)(block_ + second_len - C_SAFE_AREA_LEN) = C_SAFE_AREA_VAL;
        }
        else
        {
            // copy user data
            memcpy(block_ + tail + C_SAFE_AREA_LEN + C_HEAD_SIZE, data, first_len - C_SAFE_AREA_LEN + second_len);

            //append "SPPX" to the data
            if (second_len == 3)
            {
                *(char*)(block_ + blocksize_ - 1) = 'S';
                *(char*)(block_) = 'P';
                *(char*)(block_ + 1) = 'P';
                *(char*)(block_ + 2) = 'X';
            }
            else if (second_len == 2)
            {
                *(char*)(block_ + blocksize_ - 2) = 'S';
                *(char*)(block_ + blocksize_ - 1) = 'P';
                *(char*)(block_) = 'P';
                *(char*)(block_ + 1) = 'X';
            }
            else if (second_len == 1)
            {
                *(char*)(block_ + blocksize_ - 3) = 'S';
                *(char*)(block_ + blocksize_ - 2) = 'P';
                *(char*)(block_ + blocksize_ - 1) = 'P';
                *(char*)(block_) = 'X';
            }
        }

        //recalculate tail_ position
        // fixed by ericsha 2010-04-23; maybe make share memory queue confused
        /*
        *tail_ += (data_len + 2 * C_SAFE_AREA_LEN + C_HEAD_SIZE);
        *tail_ -= blocksize_;
        */
        int offset = (int)total_len - (int)blocksize_;
        *tail_ += offset;
    }

    //  fourth, if tail space < 4+8
    else
    {
        //append first part of head, copy tail byte
        memcpy(block_ + tail, psHead, tail_len);

        //append second part of head, copy 8-tail byte
        unsigned second_len = C_SAFE_AREA_LEN + C_HEAD_SIZE - tail_len;
        memcpy(block_, psHead + tail_len, second_len);

        //append data
        memcpy(block_ + second_len, data, data_len);

        //append C_SAFE_AREA_VAL
        *(unsigned*)(block_ + second_len + data_len) = C_SAFE_AREA_VAL;

        //recalculate tail_ position
        *tail_ = second_len + data_len + C_SAFE_AREA_LEN;
    }

    // 数据包计数
    return atomic_add_return(1, &(pstat_->msg_count));
}

int CShmMQ::dequeue(void* buf, unsigned buf_size, unsigned& data_len, unsigned& flow)
{
    unsigned head = *head_;
    unsigned tail = *tail_;

    // check if head & tail is valid
    if ( unlikely( !do_check(head, tail) ) )
    {
        data_len = 0;
        return COMMU_ERR_CHKFAIL;
    }

    if (head == tail)
    {
        data_len = 0;
        return COMMU_ERR_MQEMPTY;
    }

    atomic_sub(1, &(pstat_->msg_count)); // 尽快减，以通知到竞争者

    unsigned used_len = tail > head ? tail - head : tail + blocksize_ - head;
    char sHead[C_SAFE_AREA_LEN + C_HEAD_SIZE];
    char *psHead = sHead;

    // get head
    // if head + 8 > block_size
    if (head + C_SAFE_AREA_LEN + C_HEAD_SIZE > blocksize_)
    {
        unsigned first_size = blocksize_ - head;
        unsigned second_size = C_SAFE_AREA_LEN + C_HEAD_SIZE - first_size;
        memcpy(psHead, block_ + head, first_size);
        memcpy(psHead + first_size, block_, second_size);
        head = second_size;
    }
    else
    {
        memcpy(psHead, block_ + head, C_SAFE_AREA_LEN + C_HEAD_SIZE);
        head += (C_HEAD_SIZE + C_SAFE_AREA_LEN);
    }

    //get meta data
    unsigned total_len = *(unsigned*)(&psHead[C_SAFE_AREA_LEN]);
    flow = *(unsigned*)(psHead + C_SAFE_AREA_LEN + sizeof(unsigned));

    //check safe area
    if (*(unsigned*)(psHead) != C_SAFE_AREA_VAL)
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "safe head:%u check failed\n", *(unsigned*)(psHead));

        if ( do_check(*head_, *tail_) )
        {
            // head&tail是合法的，也需要清空共享内存; 非法时，在do_check内部已经执行清空动作
            clear();
        }

        data_len = 0;

        return COMMU_ERR_CHKFAIL;
    }

    /*解决共享内存乱问题
     * 当内存乱掉（total_len <= used_le)的时候，let head:=tail,
     * 也就是共享内存清空
     * start*/

    //assert(total_len <= used_len);
    if (unlikely(total_len > used_len))
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "Error in shm queue, key: %x, size: %d, id: %d.total_len %u,usedlen_len %u \n", 
            shmkey_, shmsize_, shmid_, total_len, used_len);

        if ( do_check(*head_, *tail_) )
        {
            // 清空共享内存
            clear();
        }

        data_len = 0;
        return COMMU_ERR_MQEMPTY;
    }

    data_len = total_len - C_HEAD_SIZE - C_SAFE_AREA_LEN;

    if (unlikely(data_len > buf_size))
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "data_len:%u > buf_size:%u\n", data_len, buf_size);

        if ( do_check(*head_, *tail_) )
        {
            // 清空共享内存
            clear();
        }

        data_len = 0;

        return COMMU_ERR_OTFBUFF;
    }

    if (head + data_len > blocksize_)
    {
        unsigned first_size = blocksize_ - head;
        unsigned second_size = data_len - first_size;

        memcpy(buf, block_ + head , first_size);
        memcpy(((char*)buf) + first_size, block_ , second_size);

        *head_ = second_size;
    }
    else
    {
        memcpy(buf, block_ + head , data_len);
        *head_ = head + data_len;
    }

    data_len -= C_SAFE_AREA_LEN;

    if (*(unsigned*)(((char*)buf) + data_len) != C_SAFE_AREA_VAL)
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "safe end check failed\n");

        if ( do_check(*head_, *tail_) )
        {
            // 清空共享内存
            clear();
        }

        data_len = 0;

        return COMMU_ERR_CHKFAIL;
    }

    pstat_->curflow = flow;
    /* 下面只在出队时更新，可不用原子 */
    atomic_add(1, &(pstat_->process_count));

    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////
CShmProducer::CShmProducer(): mq_(NULL), notify_fd_(0)
{
}

CShmProducer::~CShmProducer()
{
    fini();
    if ( notify_fd_ > 0 )
    {
        close( notify_fd_ );
    }
}

int CShmProducer::init(int shmkey, int shmsize)
{
    fini();

    mq_ = new CShmMQ;
    return mq_->init(shmkey, shmsize);
}

void CShmProducer::fini()
{
    if (mq_ != NULL)
    {
        delete mq_;
        mq_ = NULL;
    }

}

int CShmProducer::clear()
{
    int ret = 0;
    ret = mq_->clear();
    if (ret)
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "producer clear shm error, ret:%d\n", ret);
    }
    return ret;
}

int CShmProducer::produce(const void* data, unsigned data_len, unsigned flow)
{
    int ret = 0;
    ret = mq_->enqueue(data, data_len, flow);

    // notify
    if ( unlikely(ret == 1 && notify_fd_ > 0) )
    {
        if ( write(notify_fd_, "!", 1) < 0 )
        {
            INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "notify error %m\n");
        }
    }

    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////
CShmProducerL::CShmProducerL()
{
    mfd_ = 0;
}
CShmProducerL::~CShmProducerL()
{
    fini();
}

int CShmProducerL::init(int shmkey, int shmsize)
{
    fini();

    int ret = CShmProducer::init(shmkey, shmsize);
    if ( ret != 0 )
    {
        return ret;
    }

    char mfile[128] = {0};
    snprintf(mfile, sizeof(mfile) - 1, "../bin/.mq_producer_%x.lock", shmkey);
    mfd_ = open(mfile, O_RDWR | O_CREAT , 0666);
    if ( mfd_ < 0 )
    {
        return COMMU_ERR_FILEOPEN;
    }
    return 0;
}

void CShmProducerL::fini()
{
    CShmProducer::fini();

    if ( mfd_ > 0 )
    {
        close(mfd_);
    }
}

int CShmProducerL::clear()
{
    int ret = 0;

    ret = flock(mfd_, LOCK_EX);
    if (unlikely(ret != 0))
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "producer lock error %m\n");
        return COMMU_ERR_FLOCK;
    }

    ret = CShmProducer::clear();
    if (unlikely(ret != 0))
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "producer clear shm error, ret:%d\n", ret);
    }

    flock(mfd_, LOCK_UN);

    return ret;
}

int CShmProducerL::produce(const void* data, unsigned data_len, unsigned flow)
{
    int ret = flock(mfd_, LOCK_EX);
    if ( likely( ret == 0 ) )
    {
        ret = CShmProducer::produce(data, data_len, flow);
        flock(mfd_, LOCK_UN);
        return ret;
    }
    return COMMU_ERR_FLOCK;
}

/////////////////////////////////////////////////////////////////////////////////////////////
CShmComsumer::CShmComsumer(): mq_(NULL)
{
}
CShmComsumer::~CShmComsumer()
{
    fini();
}
int CShmComsumer::init(int shmkey, int shmsize)
{
    fini();

    mq_ = new CShmMQ;
    return mq_->init(shmkey, shmsize);
}
void CShmComsumer::fini()
{
    if (mq_ != NULL)
    {
        delete mq_;
        mq_ = NULL;
    }
}
int CShmComsumer::clear()
{
    int ret = 0;
    ret = mq_->clear();
    if (ret)
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "producer clear shm error, ret:%d\n", ret);
    }
    return ret;
}
inline int CShmComsumer::comsume(void* buf, unsigned buf_size, unsigned& data_len, unsigned& flow)
{
    return mq_->dequeue(buf, buf_size, data_len, flow);
}

/////////////////////////////////////////////////////////////////////////////////////////////
CShmComsumerL::CShmComsumerL()
{
    mfd_ = 0;
}
CShmComsumerL::~CShmComsumerL()
{
    fini();
}
int CShmComsumerL::init(int shmkey, int shmsize)
{
    fini();

    int ret = CShmComsumer::init(shmkey, shmsize);
    if ( ret != 0 )
    {
        return ret;
    }

    char mfile[128] = {0};
    snprintf(mfile, sizeof(mfile) - 1, "../bin/.mq_comsumer_%x.lock", shmkey);
    mfd_ = open(mfile, O_RDWR | O_CREAT , 0666);
    if ( mfd_ < 0 )
    {
        return COMMU_ERR_FILEOPEN;
    }
    return 0;

}
void CShmComsumerL::fini()
{
    CShmComsumer::fini();

    if ( mfd_ > 0 )
    {
        close(mfd_);
    }

}

int CShmComsumerL::clear()
{
    int ret = 0;

    ret = flock(mfd_, LOCK_EX);
    if (unlikely(ret != 0))
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "comsumer mutex lock error %m");
        return COMMU_ERR_FLOCK;
    }

    ret = CShmComsumer::clear();
    if (unlikely(ret != 0))
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "comsumer clear shm error, ret:%d\n", ret);
    }

    flock(mfd_, LOCK_UN);

    return ret;
}

inline int CShmComsumerL::comsume(void* buf, unsigned buf_size, unsigned& data_len, unsigned& flow)
{
    if (mq_->isEmpty())
    {
        return COMMU_ERR_MQEMPTY;
    }

    if (flock(mfd_, LOCK_EX) == 0)
    {
        int ret = mq_->dequeue(buf, buf_size, data_len, flow);
        flock(mfd_, LOCK_UN);
        return ret;
    }
    return COMMU_ERR_FLOCK;
}

/////////////////////////////////////////////////////////////////////////////////////////////
CTShmCommu::CTShmCommu(): producer_(NULL), comsumer_(NULL), locktype_(0)
{
    buff_blob_.len = 0;
    buff_blob_.data = NULL;
    buff_blob_.owner = NULL;
    buff_blob_.extdata = NULL;
}
CTShmCommu::~CTShmCommu()
{
    fini();
}
void CTShmCommu::fini()
{
    if (producer_ != NULL)
    {
        delete producer_;
        producer_ = NULL;
    }

    if (comsumer_ != NULL)
    {
        delete comsumer_;
        comsumer_ = NULL;
    }

    if (buff_blob_.data != NULL)
    {

        free(buff_blob_.data);
        buff_blob_.data = NULL;
    }
}

int CTShmCommu::init(const void* config)
{
    fini();
    TShmCommuConf* conf = (TShmCommuConf*)config;

    int ret = 0;
    locktype_ = conf->locktype_;

    //创建生产者
    if (locktype_ & LOCK_TYPE_PRODUCER)
        producer_ = new CShmProducerL;
    else
        producer_ = new CShmProducer;

    producer_->set_notify( conf->notifyfd_ );

    //创建消费者
    if (locktype_ & LOCK_TYPE_COMSUMER)
        comsumer_ = new CShmComsumerL;
    else
        comsumer_ = new CShmComsumer;



    //初始化其他变量
    buff_size_ = conf->shmsize_comsumer_;	//最大不会超过comsumer使用的共享内存大小
    buff_blob_.len = buff_size_;
    buff_blob_.data = (char *)malloc(sizeof(char) * buff_size_);
    buff_blob_.owner = this;
    buff_blob_.extdata = NULL;
    //buff_blob_.extdata = malloc( sizeof(TConnExtInfo) );

    ret = producer_->init(conf->shmkey_producer_, conf->shmsize_producer_);
    producer_->mq_->msg_timeout = conf->msg_timeout;

    if (!ret)
    {
        ret = comsumer_->init(conf->shmkey_comsumer_, conf->shmsize_comsumer_);
        comsumer_->mq_->msg_timeout = conf->msg_timeout;
    }

    if (ret != 0)
    {
        exit(-1);
    }

    return ret;
}

int CTShmCommu::clear()
{
    int ret = 0;

    ret = producer_->clear();
    if (ret != 0)
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "producer clear shm error, ret:%d.\n", ret);
        return ret;
    }
    ret = comsumer_->clear();
    if (ret != 0)
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "cosumer clear shm error, ret:%d.\n", ret);
        return ret;
    }

    return ret;
}

int CTShmCommu::poll(bool block)
{
    int processed = 0;
    unsigned flow = 0;

    do
    {
        int ret = comsumer_->comsume(buff_blob_.data, buff_size_, (unsigned&)buff_blob_.len, flow);
        if (0 != ret)
        {
            break; // when empty or error
        }
        ++processed;

        func_list_[CB_RECVDATA](flow, &buff_blob_, func_args_[CB_RECVDATA]);
    }
    while (block);

    return processed;
}

int CTShmCommu::sendto(unsigned flow, void* arg1, void* arg2)
{
    int ret = 0;
    blob_type* blob = (blob_type*)arg1;

    ret = producer_->produce(blob->data, blob->len, flow);
    INTERNAL_LOG->LOG_P_FILE(LOG_DEBUG, "sendto return [%d]\n", ret);

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

int CTShmCommu::ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2)
{
    switch (type)
    {
    case CT_STAT:
    {
        TMQStat mq;
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

    return 0;
}

int CTShmCommu::get_msg_count()
{
    /*
    TMQStat mq;
    comsumer_->getstat(mq);
    return mq.totallen_;*/
    return 0;
}
