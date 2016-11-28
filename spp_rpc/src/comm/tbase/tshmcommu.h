
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


#ifndef _TBASE_TSHMCOMMU_H_
#define _TBASE_TSHMCOMMU_H_
#include <map>
#include <sys/file.h> // flock
#include "tcommu.h"
#include "tlog.h"
#include "../singleton.h"
#include "hex_dump.h"
#include "../serverbase.h"

#define LOCK_TYPE_NONE		0x0		//不锁
#define LOCK_TYPE_PRODUCER	0x1		//写锁
#define LOCK_TYPE_COMSUMER	0x2		//读锁

#define COMMU_ERR_SHMGET	-101		//获取共享内存出错
#define COMMU_ERR_SHMNEW	-102		//创建共享内存出错
#define COMMU_ERR_SHMMAP	-103		//共享内存映射出错

#define COMMU_ERR_FILEOPEN 	-111 		//打开文件出错
#define COMMU_ERR_FILEMAP	-112		//文件映射出错
#define COMMU_ERR_MQFULL	-121		//管道满
#define COMMU_ERR_MQEMPTY  	-122		//管道空
#define COMMU_ERR_OTFBUFF	-123		//缓冲区溢出
#define COMMU_ERR_SHM_NULL	-124		//shmmem_指针为空
#define COMMU_ERR_CHKFAIL   -126		//数据校验失败
#define COMMU_ERR_MSG_TIMEOUT   -127	//共享内存数据包超时

#define COMMU_ERR_FLOCK     -201

using namespace tbase::tcommu;
using namespace tbase::tlog;
using namespace spp::singleton;

namespace tbase
{
namespace tcommu
{
namespace tshmcommu
{

//共享内存管道统计
typedef struct tagMQStat
{
    unsigned usedlen_;      //共享内存管道已用长度
    unsigned freelen_;      //共享内存管道空闲长度
    unsigned totallen_;     //共享内存管道总长度
    unsigned shmkey_;       //共享内存key
    unsigned shmid_;        //共享内存id
    unsigned shmsize_;      //共享内存大小
} TMQStat;

typedef struct qstatinfo {
    atomic_t msg_count;     // msg count in the queue
    atomic_t process_count; // processed count last interval
    atomic_t flag;          // need_notify flag
    atomic_t curflow;       // current flow
} Q_STATINFO;

//共享内存管道
class CShmMQ
{
public:
    CShmMQ();
    ~CShmMQ();
    int init(int shmkey, int shmsize);
    int clear();	//clear shm, add by aoxu, 2009-01-06
    int enqueue(const void* data, unsigned data_len, unsigned flow);
    int dequeue(void* buf, unsigned buf_size, unsigned& data_len, unsigned& flow);

    inline void* memory()
    {
        return shmmem_;
    }

    inline void getstat(TMQStat& mq_stat)
    {
        unsigned head = *head_;
        unsigned tail = *tail_;

        mq_stat.usedlen_ = (tail >= head) ? tail - head : tail + blocksize_ - head;
        mq_stat.freelen_ = head > tail ? head - tail : head + blocksize_ - tail;
        mq_stat.totallen_ = blocksize_;
        mq_stat.shmkey_ = shmkey_;
        mq_stat.shmid_ = shmid_;
        mq_stat.shmsize_ = shmsize_;
    }

    inline bool __attribute__((always_inline)) isEmpty()
    {
        return pstat_->msg_count == 0;
    }

protected:
    int shmkey_;
    int shmsize_;
    int shmid_;
    void* shmmem_;

    Q_STATINFO* pstat_;

    volatile unsigned* head_;
    volatile unsigned* tail_;
    volatile pid_t* pid_;       // pid of process doing clear operation

    char* block_;
    unsigned blocksize_;

    bool do_check(unsigned head, unsigned tail); // check if head & tail is valid
    int getmemory(int shmkey, int shmsize);
    void fini();

public:
    unsigned msg_timeout;
};

//生产者（不带锁）
class CShmProducer
{
public:
    CShmProducer();
    virtual ~CShmProducer();
    virtual int init(int shmkey, int shmsize);
    virtual int clear();

    virtual void set_notify(int fd)
    {
        notify_fd_  = fd;
    }

    virtual int produce(const void* data, unsigned data_len, unsigned flow);
    virtual void getstat(TMQStat& mq_stat)
    {
        mq_->getstat(mq_stat);
    }
    virtual void fini();

public:
    CShmMQ* mq_;
    int notify_fd_;
};
//生产者（带锁）
class CShmProducerL : public CShmProducer
{
public:
    CShmProducerL();
    ~CShmProducerL();
    int init(int shmkey, int shmsize);
    int clear();
    int produce(const void* data, unsigned data_len, unsigned flow);
protected:
    int mfd_;
    void fini();
};

//消费者（不带锁）
class CShmComsumer
{
public:
    CShmComsumer();
    virtual ~CShmComsumer();
    virtual int init(int shmkey, int shmsize);
    virtual int clear();
    virtual inline int comsume(void* buf, unsigned buf_size, unsigned& data_len, unsigned& flow);
    virtual void getstat(TMQStat& mq_stat)
    {
        mq_->getstat(mq_stat);
    }

    virtual void fini();
public:
    CShmMQ* mq_;
};
//消费者（带锁）
class CShmComsumerL : public CShmComsumer
{
public:
    CShmComsumerL();
    ~CShmComsumerL();
    int init(int shmkey, int shmsize);
    int clear();
protected:
    int mfd_;
    inline int comsume(void* buf, unsigned buf_size, unsigned& data_len, unsigned& flow);
    void fini();
};

//共享内存通讯组件
//一个发数据管道，称为生产者producer
//一个收数据管道，称为消费者comsumer
//回调函数类型：typedef int (*cb_func) (unsigned flow, void* arg1, void* arg2);(参考tcommu.h)
//flow: 数据包唯一标示；arg1: 数据blob；arg2: 用户自定义参数
//必须注册CB_RECVDATA回调函数
typedef struct
{
    int shmkey_producer_;		//发数据共享内存key
    int shmsize_producer_;		//发数据共享内存size
    int shmkey_comsumer_;		//收数据共享内存key
    int shmsize_comsumer_;		//收数据共享内存size
    int locktype_;		        //锁类型，不锁、写锁、读锁
    int maxpkg_;				//每秒最大包量, 如果为0表示不检查
    unsigned msg_timeout;   //消息在队列最长时间,expiretime_无用!
    int groupid;      //对应此消息队列的worker groupid
    int notifyfd_;              //producer 通知fd
} TShmCommuConf;


class CTShmCommu : public CTCommu
{
public:
    CTShmCommu();
    ~CTShmCommu();
    int init(const void* config);
    int clear();
    int poll(bool block = false);
    int sendto(unsigned flow, void* arg1/*数据blob*/, void* arg2/*暂时保留*/);
    int ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2);
    int get_msg_count();
protected:
    CShmProducer* producer_;
    CShmComsumer* comsumer_;
    unsigned buff_size_;
    blob_type buff_blob_;
    int locktype_;
    unsigned  msg_timeout;
    void fini();
};

}
}
}

#endif
