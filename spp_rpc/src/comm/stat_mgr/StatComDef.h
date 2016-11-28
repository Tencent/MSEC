
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


#ifndef __STAT_COMDEF_H__
#define __STAT_COMDEF_H__

#include "atomic.h"
#include "global.h"

#define BEGIN_SPP_STAT_NS namespace SPP_STAT_NS {
#define END_SPP_STAT_NS }
#define USING_SPP_STAT_NS using namespace SPP_STAT_NS

BEGIN_SPP_STAT_NS

#define MAX_CMD_NUM 102
#define MAX_CODE_NUM 30
#define MAX_DESC_BUF_SIZE 32
#define MAX_TIMEMAP_SIZE 14

#define STAT_MGR_MAGICNUM 0x32fa4190

#define MIN_PLUGIN_CMD_NUM 0
#define MAX_PLUGIN_CMD_NUM (MAX_CMD_NUM - 2) 

#define FRAME_REQ_STAT_CMD_ID 100
#define FRAME_REQ_STAT_CODE_ID 0
#define FRAME_CODE_STAT_CMD_ID 101

#define FRAME_REQ_STAT_CMD_DESC "插件处理请求时延/毫秒"
#define FRAME_CODE_STAT_CMD_DESC "SPP异步框架错误码"

#define SPP_STAT_MGR_FILE    "../stat/spp_worker_cost_stat%d.dat"
enum enumErrno
{
    EStatMgr_Success    =  0,
    EStatMgr_MagicCheck = -1,
    EStatMgr_AccessFile = -2,
    EStatMgr_OpenFile   = -3,
    EStatMgr_TrunkFile  = -4,
    EStatMgr_MapFile    = -5,
    EStatMgr_NotInit    = -6,
    EStatMgr_EvalidCmdID = -7,
    EStatMgr_EvalidCodeID= -8,
    EStatMgr_CmdObjNotInit = -9,
};

struct CodeObj
{
    bool     _used;
    unsigned _code; 
    char _desc[MAX_DESC_BUF_SIZE];
    atomic_t _count; //累加次数
    atomic_t _tmcostsum; //时延总和
    atomic_t _timecost[MAX_TIMEMAP_SIZE]; //区间时延 1-100ms step为20ms, 100-500ms step 为50ms, >500ms, 一共需要14个. 
};

struct CmdObj
{
    bool     _used;
    unsigned _cmdid;
    char     _desc[MAX_DESC_BUF_SIZE]; 
    CodeObj _codeobjs[MAX_CODE_NUM];
};
struct StatMgrPool
{
    int    _magic; 
    CmdObj _cmdobjs[MAX_CMD_NUM];
};
#define GET_MGR_POOL_SIZE sizeof(StatMgrPool)

#define STATMGR_VAL_INC(val)        atomic_inc(&(val))
#define STATMGR_VAL_DEC(val)        atomic_dec(&(val))
#define STATMGR_VAL_ADD(val, lv)    atomic_add(lv, &(val))
#define STATMGR_VAL_SET(val, lv)    atomic_set(&(val), lv)
#define STATMGR_VAL_READ(val)       atomic_read(&(val))


END_SPP_STAT_NS
#endif

