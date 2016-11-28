
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


#include "StatMgr.h"
#include <sys/file.h>
#include <sys/mman.h>
#if !__GLIBC_PREREQ(2, 3)
#define __builtin_expect(x, expected_value) (x)
#endif
#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#define CMD_OBJ_ENTRY(cmdobj, cmdid) cmdobj = &(_pool->_cmdobjs[cmdid]) 
#define CODE_OBJ_ENTRY(codeobj, cmdid, code) codeobj = &(_pool->_cmdobjs[cmdid]._codeobjs[code]) 
USING_SPP_STAT_NS;
CStatMgr::CStatMgr()
{
    _pool = NULL; 
}
CStatMgr::~CStatMgr()
{
    fini();
}
void CStatMgr::fini()
{
    if (_pool != NULL)                  
    {                                   
        ::munmap(_pool, GET_MGR_POOL_SIZE); 
    }                                   
    _pool = NULL;
}
int CStatMgr::checkStatPool()
{
    if (_pool == NULL || _pool->_magic != STAT_MGR_MAGICNUM)
    {
        return EStatMgr_MagicCheck;
    }
    return 0;
}

void CStatMgr::newStatPool()
{
    memset(_pool, 0, GET_MGR_POOL_SIZE);
    _pool->_magic =  STAT_MGR_MAGICNUM;
    return;
}
bool CStatMgr::isInit()
{
    return (likely(_pool != NULL)) ? true : false; 
}
StatMgrPool* CStatMgr::getStatPool()
{
    return _pool;
}

int CStatMgr::initStatPool(const char* filepath)
{
    if (_pool != NULL) 
    {
        fini();
    }
    if (filepath == NULL )
    {
        return EStatMgr_OpenFile;
    }
    int fd = ::open(filepath, O_CREAT | O_RDWR, 0666);    
    if (unlikely(fd < 0))
    {
        return EStatMgr_OpenFile;
    }
    ::flock(fd, LOCK_EX);
    if (unlikely (ftruncate(fd, GET_MGR_POOL_SIZE) < 0))
    {
        ::close (fd);
        return EStatMgr_TrunkFile;
    }
    _pool = (StatMgrPool*) ::mmap(NULL, GET_MGR_POOL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    if (unlikely(MAP_FAILED == _pool))
    {
        ::close (fd);
        fini();
        return EStatMgr_MapFile;
    }
    if (_pool->_magic == 0)
    {
        newStatPool();
    }
    else
    {
        int ret = checkStatPool();
        if (ret != 0)
        {
            ::close(fd); 
            fini();
            ::unlink(filepath);
            return ret;
        }
    }
    ::flock(fd, LOCK_UN);
    ::close(fd);
    return 0;
}
int CStatMgr::readStatPool(const char* filepath)
{
    if (_pool != NULL) 
    {
        fini();
    }
    if (filepath == NULL || access(filepath, R_OK))
    {
        return EStatMgr_OpenFile;
    }

    int fd = ::open(filepath, O_RDONLY, 0666);    
    if (unlikely(fd < 0))
    {
        return EStatMgr_OpenFile;
    }
    _pool = (StatMgrPool*) ::mmap(NULL, GET_MGR_POOL_SIZE, PROT_READ , MAP_SHARED, fd, 0); 
    if (unlikely(MAP_FAILED == _pool))
    {
        ::close (fd);
        fini();
        return EStatMgr_MapFile;
    }
    int ret = checkStatPool();
    if (ret != 0)
    {
        ::close(fd); 
        fini();
        return ret;
    }
    ::close(fd);

    return 0;
}
int CStatMgr::initCmdItem(unsigned cmdid, const char* cmddesc) 
{
    if (cmdid >= MAX_CMD_NUM)
        return EStatMgr_EvalidCmdID;
    CmdObj* cmdobj = NULL; 
    CMD_OBJ_ENTRY(cmdobj, cmdid);
    // lock or not lock?
    if (unlikely(cmdobj->_used == false))
    {
        cmdobj->_used = true;
        cmdobj->_cmdid = cmdid;
        if (cmddesc != NULL)
        {
            strncpy(cmdobj->_desc, cmddesc, MAX_DESC_BUF_SIZE - 1);
        }
    }
    return 0;
}
int CStatMgr::initFrameStatItem()
{
    if (isInit() == false)
    {
        return EStatMgr_NotInit;
    }
    //初始化请求时延统计命令字
    int ret = initCmdItem(FRAME_REQ_STAT_CMD_ID, FRAME_REQ_STAT_CMD_DESC);
    initCmdCodeItem(FRAME_REQ_STAT_CMD_ID, FRAME_REQ_STAT_CODE_ID);  
    if (ret != 0)
    {
        return ret;
    }

    ret = initCmdItem(FRAME_CODE_STAT_CMD_ID, FRAME_CODE_STAT_CMD_DESC);
    for (int i = 0; i <= 14 ; ++ i)
    {
        initCmdCodeItem(FRAME_CODE_STAT_CMD_ID, i);
    }
    return ret;
}
int CStatMgr::initStatItem(unsigned cmdid, const char* cmddesc )
{
    if (isInit() == false)
    {
        return EStatMgr_NotInit;
    }
    if (cmdid >= MAX_PLUGIN_CMD_NUM)
    {
        return EStatMgr_EvalidCmdID;
    }
    return initCmdItem(cmdid, cmddesc); 
}

int CStatMgr::stepStatItem(const unsigned& cmdid, 
                           const unsigned& code, 
                           const unsigned& timecost, 
                           const char* codedesc )
{
    if (unlikely(isInit() == false))
    {
        return EStatMgr_NotInit;
    }
    if (unlikely(cmdid >= MAX_PLUGIN_CMD_NUM))
    {
        return EStatMgr_EvalidCmdID;
    }
    if (unlikely(code >= MAX_CODE_NUM))
    {
        return EStatMgr_EvalidCodeID;
    }
    
    return stepItem(cmdid, code, timecost, codedesc);

}
int CStatMgr::stepReqStat(const unsigned& timecost)
{
    if (unlikely(isInit() == false))
    {
        return EStatMgr_NotInit;
    }
    return stepItem(FRAME_REQ_STAT_CMD_ID, FRAME_REQ_STAT_CODE_ID, timecost);
}
int CStatMgr::stepCodeStat(const unsigned& code, const char * desc)
{
    if (unlikely(isInit() == false))
    {
        return EStatMgr_NotInit;
    } 
    if (code >= MAX_CODE_NUM)
    {
        return EStatMgr_EvalidCodeID;
    }
    return stepItem(FRAME_CODE_STAT_CMD_ID, code, 0, desc);
}
int CStatMgr::stepItem(const unsigned& cmdid,     
                       const unsigned& code,      
                       const unsigned& timecost,  
                       const char* codedesc)
{
    CmdObj *cmdobj = NULL;
    CMD_OBJ_ENTRY(cmdobj, cmdid);
    if (cmdobj->_used == false)
        return EStatMgr_CmdObjNotInit;
    CodeObj* codeobj = NULL;

    CODE_OBJ_ENTRY(codeobj, cmdid, code);
    if (unlikely(codeobj->_used == false))
    {
        codeobj->_used = true;
        codeobj->_code = code;
        if (codedesc != NULL)
            strncpy(codeobj->_desc, codedesc, MAX_DESC_BUF_SIZE - 1);
    }

    STATMGR_VAL_INC(codeobj->_count);
    STATMGR_VAL_ADD(codeobj->_tmcostsum, timecost);
    STATMGR_VAL_INC(codeobj->_timecost[getTimeIndex(timecost)]);
    return 0;

}
int CStatMgr::getTimeIndex(const unsigned& timecost)
{
    if (timecost >= 500)
        return MAX_TIMEMAP_SIZE - 1;
    else if (timecost >= 100)
    {
        return (timecost - 100) / 50 + 5; 
    }
    else
    {
        return timecost / 20;
    }
}
void CStatMgr::initCmdCodeItem(unsigned cmdid, unsigned code)
{
    CmdObj *cmdobj = NULL;
    CMD_OBJ_ENTRY(cmdobj, cmdid);
    if (cmdobj->_used == false)
        return;
    CodeObj* codeobj = NULL;

    CODE_OBJ_ENTRY(codeobj, cmdid, code);
    if (unlikely(codeobj->_used == false))
    {
        codeobj->_used = true;
        codeobj->_code = code;
    }
}
