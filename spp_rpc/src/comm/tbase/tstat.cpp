
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
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "tstat.h"
#include "misc.h"

//Macro for compiler optimization
#if !__GLIBC_PREREQ(2, 3)
#define __builtin_expect(x, expected_value) (x)
#endif
#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

using namespace tbase::tstat;

#define OBJ_INIT(obj, id, type, desc) 			\
do {												\
	memset(obj->id_, 0x0, sizeof(obj->id_));		\
	strncpy(obj->id_, id, sizeof(obj->id_) - 1);	\
	if(desc)										\
		strncpy(obj->desc_, desc, sizeof(obj->desc_) - 1);\
	else	\
		memset(obj->desc_, 0x0, sizeof(obj->desc_));\
	obj->type_ = type;								\
	obj->next_ = INVALID_HANDLE;					\
}while(0)

#define OBJ_ENTRY(obj, ent)	obj = &statpool_->statobjs_[ent];
#define OBJ_WRAPPER(obj, wrapper) \
do {					\
	wrapper.id_ = obj->id_;  \
	wrapper.desc_ = obj->desc_; \
	wrapper.type_ = obj->type_; \
	wrapper.val_size_ = obj->val_size_;\
	wrapper.count_ = &obj->count_; \
	wrapper.value_ = statpool_->statvals_ + obj->val_offset_; \
}while(0)

#define STAT_ERR_MAGIC		-200
#define STAT_ERR_VERSION	-201
#define STAT_ERR_FILESTAT	-202
#define STAT_ERR_SIZE		-203

//stat data buff
typedef struct
{
    char* buffer;
    int*  len;
    int  size;
} cdata;

CTStat::CTStat() : statpool_(NULL), policy_num_(0), mapfile_(false)
{
    memset(policy_, 0x0, STAT_TYPE_NUM * sizeof(long));
    memset(policy_no_, 0x0, STAT_TYPE_NUM * sizeof(int));
    memset(policy_type_, 0x0, (1 << STAT_TYPE_NUM) * sizeof(int));
    memset(statindex_, 0x0 , DEFAULT_STATOBJ_NUM * sizeof(TStatObj *));

    init_statpolicy(new CTStatPolicySum, STAT_TYPE_SUM);
    init_statpolicy(new CTStatPolicyAvg, STAT_TYPE_AVG);
    init_statpolicy(new CTStatPolicyMax, STAT_TYPE_MAX);
    init_statpolicy(new CTStatPolicyMin, STAT_TYPE_MIN);
    init_statpolicy(new CTStatPolicyCount, STAT_TYPE_COUNT);
    init_statpolicy(new CTStatPolicySet, STAT_TYPE_SET);
}

CTStat::~CTStat()
{
    for (int i = 0; i < STAT_TYPE_NUM; ++i)
    {
        if (policy_[i])
        {
            delete policy_[i];
        }
    }

    if (statpool_)
    {
        if (mapfile_)
        {
            munmap(statpool_, sizeof(TStatPool));
        }
        else
            free(statpool_);
    }
}

int CTStat::check_statpool(const char* mapfilepath)
{
    if (unlikely(statpool_->magic_ != TSTAT_MAGICNUM))
    {
        fprintf(stderr, "Stat mapfile:%s head magic number error, magic=%lld\n", mapfilepath, statpool_->magic_);
        return STAT_ERR_MAGIC;
    }

    return 0;
}

int CTStat::init_statpool(const char* mapfilepath)
{
    //如果文件路径为空，使用进程堆内存
    if (!mapfilepath)
    {
        mapfile_ = false;
        statpool_ = (TStatPool*)malloc(sizeof(TStatPool));
        newpool();	//初始化统计池
        //否则使用文件映射的共享内存
    }
    else
    {
        mapfile_ = true;

        int fd = open(mapfilepath, O_CREAT | O_RDWR, 0666);

        if (unlikely(fd < 0))
            return ERR_STAT_OPENFILE;

        flock(fd, LOCK_EX); // close时会自动解锁

        if (unlikely(ftruncate(fd, sizeof(TStatPool)) < 0))
        {
            flock(fd, LOCK_UN);
            close(fd);
            return ERR_STAT_TRUNFILE;
        }

        statpool_ = (TStatPool*)mmap(NULL, sizeof(TStatPool), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if (unlikely(MAP_FAILED == statpool_))
        {
            flock(fd, LOCK_UN);
            close(fd);
            statpool_ = NULL;
            return ERR_STAT_MAPFILE;
        }

        if ( statpool_->magic_ == 0 )
        {
            newpool();
        }
        else
        {
            int check_ret = check_statpool(mapfilepath);
            if (unlikely(check_ret < 0))
            {
                flock(fd, LOCK_UN);
                close(fd);
                munmap(statpool_, sizeof(TStatPool));
                statpool_ = NULL;
                if (unlink(mapfilepath) == -1)
                {
                    fprintf(stderr, "Delete file %s error,ErrMsg:%m\n", mapfilepath);   //delete file when it's not right
                }
                return check_ret;
            }
        }
        flock(fd, LOCK_UN);
        close(fd);
    }

    return 0;
}

int CTStat::read_statpool(const char* mapfilepath)
{
    if (!mapfilepath)
    {
        fprintf(stderr, "mapfilepath can not be NULL\n");
        return ERR_STAT_FILE_NULL;
    }

    mapfile_ = true;

    if (access(mapfilepath, R_OK))
    {
        return ERR_STAT_TRUNFILE;
    }

    int fd = open(mapfilepath, O_RDONLY, 0666);

    if (unlikely(fd < 0))
        return ERR_STAT_OPENFILE;

    statpool_ = (TStatPool*)mmap(NULL, sizeof(TStatPool), PROT_READ , MAP_SHARED, fd, 0);

    if (unlikely(MAP_FAILED == statpool_))
    {
        close(fd);
        statpool_ = NULL;
        return ERR_STAT_MAPFILE;
    }

    close(fd);

    //校验statpool头部
    if (unlikely(check_statpool(mapfilepath) < 0))
    {
        close(fd);
        munmap(statpool_, sizeof(TStatPool));
        statpool_ = NULL;
        return ERR_STAT_MEMERROR;
    }

    return 0;
}

int CTStat::init_statpolicy(CTStatPolicy* policy, int type)
{
#ifndef SPP_UNITTEST

    //assert(policy_num_ < STAT_TYPE_NUM);
    if (policy_num_ >= STAT_TYPE_NUM)
    {
        printf("[ERROR]stat policy type number:%d >= max stat type number:%d.\n",
               policy_num_,
               STAT_TYPE_NUM
              );
        exit(-1);
    }

#else

    if (policy_num_ >= STAT_TYPE_NUM)
        return -1;

#endif
    int i = 0;
    int j = type;

    while ((j = (j >> 1)))
    {
        ++i;
    }

    policy_no_[i] = type;
    policy_[i] = policy;
    policy_type_[type] = i;
    policy_num_++;
    return 0;
}

//把TStatObj指针数组按index赋值
int CTStat::set_statindex(int index, TStatObj *obj)
{
    if (index >= 0 && index < DEFAULT_STATOBJ_NUM)
    {
        statindex_[index] = obj;
        return 0;
    }
    else
    {
        return -1;
    }
}

//根据index快速获取TStatObj
TStatObj* CTStat::get_statindex(int index)
{
    if (index >= 0 && index < DEFAULT_STATOBJ_NUM)
    {
        return statindex_[index];
    }
    else
    {
        return NULL;
    }
}

int CTStat::init_statobj(const char* id, int type, const char* desc, int val_size)
{
    if (find_statobj(id) != INVALID_HANDLE)
        return ERR_STAT_EXIST;

    TStatObj* obj = NULL;
    int choice = INVALID_HANDLE;

    for (int i = 0; i < STAT_TYPE_NUM; ++i)
    {
        if (type & policy_no_[i])
        {
            choice = alloc_statobj(val_size);

            if (choice != INVALID_HANDLE)
            {
                OBJ_ENTRY(obj, choice);
                OBJ_INIT(obj, id, policy_no_[i], desc);
                insert_statobj(choice);
            }
            else
                return ERR_STAT_FULL;
        }
    }

    return 0;
}

//性能优化版的统计对象初始化方法，只在框架内使用
//唯一的区别在于多传一个index参数
int CTStat::init_statobj_frame(const char* id, int type, int index, const char* desc, int val_size)
{
    TStatObj* obj = NULL;
    if (find_statobj(id, STAT_TYPE_ALL, &obj) != INVALID_HANDLE)
    {
        set_statindex(index, obj);	//初始化统计对象索引
        return ERR_STAT_EXIST;
    }

    int choice = INVALID_HANDLE;
    //printf("stat not exist\n");

    for (int i = 0; i < STAT_TYPE_NUM; ++i)
    {
        if (type & policy_no_[i])
        {
            choice = alloc_statobj(val_size);

            if (choice != INVALID_HANDLE)
            {
                //printf("stat choice is not invalid handle\n");
                OBJ_ENTRY(obj, choice);
                OBJ_INIT(obj, id, policy_no_[i], desc);

                insert_statobj_frame(choice, index);
            }
            else
                return ERR_STAT_FULL;
        }
    }

    return 0;
}

//step0的性能优化版
int CTStat::op(int index, long val, int val_idx)
{
    TStatObj* obj = NULL;
    TStatObjWrapper wrapper;
    int policy_no = 0;

    obj = get_statindex(index);
    if (obj)
    {
        policy_no = policy_type_[obj->type_];
        OBJ_WRAPPER(obj, wrapper);
        policy_[policy_no]->__step__(&wrapper, val, val_idx);
    }

    return 0;
}

int CTStat::step(const char** ids, int num, long val, int val_idx)
{
    TStatObj* obj = NULL;
    TStatObjWrapper wrapper;
    int choice = INVALID_HANDLE;
    int policy_no = 0;

    for (int i = 0; i < num; ++i)
    {
        choice = find_statobj(ids[i]);

        while (choice != INVALID_HANDLE)
        {
            OBJ_ENTRY(obj, choice);

            if (!strcmp(obj->id_, ids[i]))
            {
                policy_no = policy_type_[obj->type_];
                OBJ_WRAPPER(obj, wrapper);
                policy_[policy_no]->__step__(&wrapper, val, val_idx);
                choice = obj->next_;
            }
            else
            {
                break;
            }
        }
    }

    return 0;
}

int CTStat::step0(const char* id, long val, int val_idx)
{
    return step(&id, 1, val, val_idx);
}

void CTStat::result(char** buffer, int* len, int size)
{
    *len = 0;
    output_statpool(*buffer, len, size);
    cdata buff = {*buffer, len, size};
    travel(&CTStat::do_result, &buff);
}

void CTStat::reset()
{
    travel(&CTStat::do_reset);
}

int CTStat::query(const char* id, TStatObjWrapper* wrapper)
{
    int choice = find_statobj(id, STAT_TYPE_ALL); //fixed me! defect, only return the first statobj matched

    if (likely(choice != INVALID_HANDLE))
    {
        TStatObj* obj = NULL;
        OBJ_ENTRY(obj, choice);
        OBJ_WRAPPER(obj, (*wrapper));
        return 1;
    }
    else
        return ERR_STAT_NONE;
}

int CTStat::queryindex(int index, TStatObjWrapper* wrapper)
{
    TStatObj* obj = get_statindex(index);
    if (likely(obj))
    {
        OBJ_WRAPPER(obj, (*wrapper));
        return 1;
    }
    else
        return ERR_STAT_NONE;
}

void CTStat::travel(visit_func visitor, void* data)
{
    int next = 0;

    for (int i = 0; i < BUCKET_NUM; ++i)
    {
        next = statpool_->bucket_[i];
        TStatObj* obj = NULL;

        while (1)
        {
            if (INVALID_HANDLE == next)
                break;
            else
            {
                OBJ_ENTRY(obj, next);
                (this->*visitor)(obj, data);
                next = obj->next_;
            }
        }
    }
}

void CTStat::do_reset(TStatObj* obj, void* data)
{
    TStatObjWrapper wrapper;
    int policy_no = 0;
    policy_no = policy_type_[obj->type_];
    OBJ_WRAPPER(obj, wrapper);
    policy_[policy_no]->__reset__(&wrapper);
}

void CTStat::do_result(TStatObj* obj, void* data)
{
    int policy_no = 0;
    long values[STAT_MAX_VALSIZE];
    long val_size = 0;
    long count = 0;
//	int* len = (int*)data;
//	char *buffer = statpool_->buffer_ + sizeof(int);
    cdata* buff = (cdata*)data;

    policy_no = policy_type_[obj->type_];
    TStatObjWrapper wrapper;
    OBJ_WRAPPER(obj, wrapper);
    count = policy_[policy_no]->__result__(&wrapper, values, &val_size);
//	output_statobj(obj, count, values, val_size, buffer, len);
    output_statobj(obj, count, values, val_size, buff->buffer, buff->len, buff->size);
}

int CTStat::find_statobj(const char* id, int type, TStatObj **pobj)
{
    int bucketid  = hashid(id);
    int next = statpool_->bucket_[bucketid];
    TStatObj* obj = NULL;

    while (1)
    {
        if (INVALID_HANDLE == next)
            break;
        else
        {
            OBJ_ENTRY(obj, next);

            if (!strcmp(obj->id_, id) && ((type == STAT_TYPE_ALL) || (type == obj->type_)))
            {
                if (pobj)
                {
                    *pobj = obj;
                }
                break;
            }
            else
            {
                if (obj->next_ == next)
                {
                    fprintf(stderr, "[ERROR] stat files(../stat/*) are damaged, please delete them and restart\n");
                    exit(-1);
                }
                next = obj->next_;
            }
        }
    }

    return next;
}

static inline void __kqstat_lock_wait(volatile int *lock)
{
    int retry = 100; /* 防止死锁 */
    while (__sync_lock_test_and_set(lock, 1) && retry--) {
        usleep(128*1024); /* 轮询等待 */
    }
}

static inline void __kqstat_unlock(int *lock)
{
    __sync_lock_release(lock);
}

int CTStat::alloc_statobj(int val_size)
{
#ifndef SPP_UNITTEST

    //assert(val_size >= 1 && statpool_->statobjs_used_ < DEFAULT_STATOBJ_NUM && statpool_->statvals_used_ < DEFAULT_STATVAL_NUM - val_size);
    if (val_size < 1 || statpool_->statobjs_used_ >= DEFAULT_STATOBJ_NUM || statpool_->statvals_used_ >= DEFAULT_STATVAL_NUM -
            val_size)
    {
        fprintf(stderr, "[ERROR]please check:\n"
                "* stat allocate val_size:%d should be >= 1\n"
                "* statobjs used:%d should be < default statobj number:%d\n"
                "* statvals used:%d should be < default statval number:%d\n",
                val_size,
                statpool_->statobjs_used_,
                DEFAULT_STATOBJ_NUM,
                statpool_->statvals_used_,
                DEFAULT_STATVAL_NUM - val_size
               );
        exit(-1);
    }

#else

    if (val_size < 1 || statpool_->statobjs_used_ > DEFAULT_STATOBJ_NUM || statpool_->statvals_used_ > DEFAULT_STATVAL_NUM - val_size)
        return INVALID_HANDLE;

#endif

    __kqstat_lock_wait(&statpool_->lock_);

    TStatObj* obj = NULL;
    int choice = statpool_->freelist_;

    OBJ_ENTRY(obj, choice);
    obj->val_size_ = val_size;
    obj->val_offset_ = statpool_->statvals_used_;
//	obj->value_ = statpool_->statvals_ + obj->val_offset_;
    statpool_->freelist_ = obj->next_;
    statpool_->statobjs_used_++;
    statpool_->statvals_used_ += val_size;

    __kqstat_unlock(&statpool_->lock_);

    return choice;
}

void CTStat::insert_statobj(int choice)
{
    TStatObj* obj = NULL;
    OBJ_ENTRY(obj, choice);

    int bucketid = hashid(obj->id_);

    __kqstat_lock_wait(&statpool_->lock_);

    int next = statpool_->bucket_[bucketid];
    obj->next_ = next;
    statpool_->bucket_[bucketid] = choice;

    __kqstat_unlock(&statpool_->lock_);
}

void CTStat::insert_statobj_frame(int choice, int index)
{
    TStatObj* obj = NULL;
    OBJ_ENTRY(obj, choice);

    __kqstat_lock_wait(&statpool_->lock_);

    set_statindex(index, obj);	//初始化统计对象索引
    int bucketid = hashid(obj->id_);
    int next = statpool_->bucket_[bucketid];
    obj->next_ = next;
    statpool_->bucket_[bucketid] = choice;

    __kqstat_unlock(&statpool_->lock_);
}

void CTStat::newpool()
{
    memset(statpool_, 0x0, sizeof(TStatPool));
    statpool_->magic_ = TSTAT_MAGICNUM;
    statpool_->freelist_ = 0;
    statpool_->statobjs_used_ = 0;
    statpool_->statvals_used_ = 0;

    int i;

    for (i = 0; i < BUCKET_NUM; ++i)
        statpool_->bucket_[i] = INVALID_HANDLE;

    for (i = 0; i < DEFAULT_STATOBJ_NUM - 1; ++i)
        statpool_->statobjs_[i].next_ = i + 1;

    statpool_->statobjs_[DEFAULT_STATOBJ_NUM - 1].next_ = INVALID_HANDLE;
}

void CTStat::output_statpool(char* buffer, int* len, int size)
{
    time_t now = time(NULL);
    struct tm tmm;
    localtime_r(&now, &tmm);

    *len += snprintf(buffer + *len, size - (*len) - 1, "\nTStat[%-5d],%04d-%02d-%02d %02d:%02d:%02d\nUsed StatObj Num: %d\tUsed StatVal Num: %d\n", (int)syscall(__NR_gettid),
                     tmm.tm_year + 1900, tmm.tm_mon + 1, tmm.tm_mday, tmm.tm_hour, tmm.tm_min, tmm.tm_sec,
                     statpool_->statobjs_used_, statpool_->statvals_used_);

    *len += snprintf(buffer + *len, size - (*len) - 1,  "%-20s|%-8s|%-8s|%-8s|%-20s|\n",
                     "StatID", "Type", "Count", "Values", "Desc");
}

void CTStat::output_statobj(
    const TStatObj* obj,
    long count,
    const long* values,
    int val_size,
    char* buffer,
    int* len,
    int buffer_size)
{
    *len += snprintf(buffer + *len, buffer_size - (*len) - 1, "%-20s|%-8s|%-8ld|",
                     obj->id_, policy_[policy_type_[obj->type_]]->__tag__(), count);

    //printf("%-20s|%-20s|%-8s|%-8ld|", obj->id_, obj->desc_, policy_[policy_type_[obj->type_]]->__tag__(), count);
    //printf("%-11ld\n", values[0]);
    for (int i = 0; i < val_size; ++i)
    {
        *len += snprintf(buffer + *len, buffer_size - (*len) - 1, "%-8ld", values[i]);
    }

    *len += snprintf(buffer + *len, buffer_size - (*len) - 1, "|%-20s", obj->desc_);
    *len += snprintf(buffer + *len, buffer_size - (*len) - 1, "\r\n");
}

