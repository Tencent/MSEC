
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


#ifndef _TBASE_TSTAT_H_
#define _TBASE_TSTAT_H_
#include <stdlib.h>
#include <string.h>
//#include <assert.h>
#include "tstat_policy.h"

#define STAT_TYPE_SUM     1				//累加操作 
#define STAT_TYPE_AVG     1<<1			//平均操作
#define STAT_TYPE_MAX     1<<2			//最大值操作
#define STAT_TYPE_MIN     1<<3			//最小值操作
#define STAT_TYPE_COUNT   1<<4			//计数操作
#define STAT_TYPE_SET     1<<5			//设置操作, add by jeremy

#define STAT_TYPE_ALL	  -1			//通配	
//modified by jeremy
#define STAT_TYPE_NUM     6				//统计策略个数

#define TSTAT_MAGICNUM    83846584		//统计池幻数 
#define TSTAT_VERSION	  0x02			//统计版本
#define BUCKET_NUM        200			//统计对象桶个数
#define DEFAULT_STATOBJ_NUM  200		//统计对象总个数
#define DEFAULT_STATVAL_NUM  (DEFAULT_STATOBJ_NUM*1)  //统计值总个数（一个统计对象至少有一个统计值）
#define STAT_BUFF_SIZE	  1<<18  		//统计报表缓冲区大小
#define STAT_MAX_VALSIZE  10            //统计值最大维数
#define INVALID_HANDLE	  -1

#define ERR_STAT_EXIST    	-2000       //统计对象已经存在
#define ERR_STAT_NONE	  	-2001		//统计对象不存在
#define ERR_STAT_FULL	  	-2010      	//没有空闲内存存储统计对象
#define ERR_STAT_OPENFILE 	-2020		//打开内存映射文件失败
#define ERR_STAT_TRUNFILE 	-2021		//truncate文件失败
#define ERR_STAT_MAPFILE  	-2030		//映射文件失败
#define ERR_STAT_MEMERROR	-2040		//共享内存区数据损坏
#define ERR_STAT_DEL_FILE	-2050		//删除统计文件失败
#define ERR_STAT_FILE_NULL	-2060		//统计文件指针为空

namespace tbase
{
namespace tstat
{
///////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    char id_[STAT_ID_MAX_LEN];			//统计ID
    char desc_[STAT_ID_MAX_LEN * 2];		//统计描述
    int type_;							//统计类型
    int val_size_;                      //统计值维数
    TStatVal count_;					//次数
    int val_offset_;                    //统计值偏移量
    int next_;							//下一个TStatObj
} TStatObj; //统计对象

///////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    long long magic_;					//幻数
    int lock_;							
    char buffer_[STAT_BUFF_SIZE];		//统计报表缓冲区 --不使用了
    int freelist_;						//空闲TStatObj链表
    int bucket_[BUCKET_NUM];			//TStatObj哈稀桶
    int statobjs_used_;					//使用的TStatObj数目
    TStatObj statobjs_[DEFAULT_STATOBJ_NUM];  //TStatObj数组
    int statvals_used_;                  	  //使用的TStatVal数目
    TStatVal statvals_[DEFAULT_STATVAL_NUM];  //TStatVal数组
} TStatPool; //统计对象池

///////////////////////////////////////////////////////////////////////////////////
//统计池类
class CTStat
{
public:
    CTStat();
    ~CTStat();
    //初始化统计池, 当mapfile != NULL，则使用文件映射的共享内存，否则使用进程堆内存
    //！！！！！警告：本方法不提供给用户使用，仅在框架里使用。
    //！！！！！如果用户要使用统计功能，请使用框架传递给插件的统计对象base->stat_
    int init_statpool(const char* mapfilepath = NULL);

    //读统计池, 统计工具使用
    int read_statpool(const char* mapfilepath = NULL);

    //初始化统计策略
    //policy:  统计策略类
    //type:    统计策略类型（2的整数次幂）
    int init_statpolicy(CTStatPolicy* policy, int type);

    //初始化统计对象
    //id:	统计ID
    //type:	统计策略类型（可以复合）
    //desc: 统计描述
    //value_size: 统计对象统计值维数
    int init_statobj(const char* id, int type, const char* desc = NULL, int val_szie = 1);

    //一次统计（多个对象）
    //ids:	统计ID数组
    //num:	统计ID个数
    //val:  统计值分量
    //value_idx: 统计值维度
    int step(const char** ids, int num, long val, int val_idx = 1);

    //一次统计（单个对象）
    //id:	统计ID
    //val:  统计值分量
    //value_idx: 统计值维度
    int step0(const char* id, long val, int val_idx = 1);

    //生成统计结果
    //buffer: 指向统计报表缓冲区的指针
    //len:	  统计报表长度
    //size:   统计报表缓冲区的大小
    void result(char** buffer, int* len, int size);

    //重置所有统计对象，清空统计值
    void reset();

    //查询统计对象信息
    //id:       统计ID
    //wrapper:  统计对象信息
    int query(const char* id, TStatObjWrapper* wrapper);

    //性能优化后的初始化统计对象，仅在框架内使用
    //id:	统计ID
    //type:	统计策略类型（可以复合）
    //index:统计对象下标/索引（在statindex_中使用)
    //desc: 统计描述
    //value_size: 统计对象统计值维数
    int init_statobj_frame(const char* id, int type, int index, const char* desc = NULL, int val_szie = 1);

    //一次统计（单个对象）（性能优化版，只在框架内使用）
    //index:	统计索引
    //val: 		统计值分量
    //value_idx:统计值维度
    int op(int index, long val, int val_idx = 1);

    //查询统计对象信息（性能优化版）
    //index:    统计索引
    //wrapper:  统计对象信息
    int queryindex(int index, TStatObjWrapper* wrapper);

protected:
    CTStatPolicy* policy_[STAT_TYPE_NUM];
    int policy_no_[STAT_TYPE_NUM];
    int policy_type_[1 << STAT_TYPE_NUM];
    TStatPool* statpool_;
    int policy_num_;
    bool mapfile_;
    TStatObj* statindex_[DEFAULT_STATOBJ_NUM];

    //JS Hash算法，把字符串hash为32位整数
    inline unsigned hashid(const char* id)
    {
        int len = strlen(id);
        unsigned hash = 1315423911;

        for (int i = 0; i < len; ++i)
        {
            hash ^= ((hash << 5) + id[i] + (hash >> 2));
        }

        return hash % BUCKET_NUM;
    }
    int check_statpool(const char* mapfilepath);
    int find_statobj(const char* id, int type = STAT_TYPE_ALL, TStatObj **pobj = NULL);
    int alloc_statobj(int val_size);
    void insert_statobj(int choice);
    void insert_statobj_frame(int choice, int index);
    void output_statpool(char* buffer, int* len, int size);
    void output_statobj(const TStatObj* obj, long count, const long* values, int val_size, char* buffer, int* len, int buffer_size);
    void newpool();
    typedef void (CTStat::*visit_func)(TStatObj*, void*);
    void travel(visit_func visitor, void* data = NULL);
    void do_reset(TStatObj* obj, void* data = NULL);
    void do_result(TStatObj* obj, void* data = NULL);

    //把TStatObj指针数组按index赋值
    //index:	索引/下标
    //obj:		init后分配的TStatObj指针
    int set_statindex(int index, TStatObj *obj);

    //根据index快速获取TStatObj
    //index:	索引/下标
    //返回值:	TStatObj指针
    TStatObj* get_statindex(int index);

};
}
}
#endif
