
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


#ifndef __CSTAT_MGR_H__
#define __CSTAT_MGR_H__

#include "ICostStat.h"
#include "IFrameStat.h"
#include "StatComDef.h"

BEGIN_SPP_STAT_NS
class CStatMgr
    : public ICostStat
    , public ICodeStat
    , public IReqStat
{
    public:
        CStatMgr();
        virtual ~CStatMgr();
        /**
         * 初始化接口, 可读可写
         *
         * @param filepath 统计文件路径 
         *
         * @return : 成功；=0：<0: 失败
         */
   
       int initStatPool(const char* filepath);
       /**
         * 初始化接口, 只读
         *
         * @param filepath 统计文件路径 
         *
         * @return : 成功；=0：<0: 失败
         */

       int readStatPool(const char* filepath);

       /**
         * 得到统计结构体 
         *
         * @return : 成功：结构体指针 失败:NULL
         */

       StatMgrPool* getStatPool();

       /*
        * 初始化框架统计
        */
       int initFrameStatItem();
        /**
         * 初始化命令字统计, cmdid的范围为[0, 99], 只有初始化过的命令字才能对返回码进行统计.
         *
         * @param cmdid    命令字
         * @param cmddesc  命令字描述信息, 可为空, 上报不会把描述信息上报. 
         *
         * @return : 成功；=0：<0: 失败
         *
         */

       int initStatItem(unsigned cmdid, const char* cmddesc = NULL); 
        /**
         * 对命令字和返回码统计. 
         *
         * @param cmdid    命令字, 范围[0, 99]
         * @param code     返回码, 范围[0, 29] 
         * @param timecost 花费时间, 单位毫秒
         * 时间区间为 1-100ms 步长为20ms, 100-500ms 步长为50ms, >500ms,  
         * @param codedesc 返回码描述信息, 可为空, 上报不会把描述信息上报 
         *
         * @return : 成功；=0：<0: 失败
         *
         */

       int stepStatItem(const unsigned& cmdid, const unsigned& code,const unsigned& timecost, const char* codedesc =NULL);
        /**
         * 框架对处理时延统计接口. 
         *
         * @param timecost 花费时间, 单位毫秒
         * 时间区间为 1-100ms 步长为20ms, 100-500ms 步长为50ms, >500ms,  
         *
         * @return : 成功；=0：<0: 失败
         *
         */

       int stepReqStat(const unsigned& timecost);
        /**
         * 框架对异步错误码统计接口. 
         *
         * @param code 异步错误码
         * @param desc 错误码描述信息 
         *
         * @return : 成功；=0：<0: 失败
         *
         */

       int stepCodeStat(const unsigned& code, const char * desc = NULL);

    protected:
        
       int initCmdItem(unsigned cmdid, const char* cmddesc = NULL);
       int stepItem(const unsigned& cmdid,const unsigned& code, const unsigned& timecost, const char* codedesc = NULL);
       void initCmdCodeItem(unsigned cmdid, unsigned code);
       inline int getTimeIndex(const unsigned& timecost);
       void fini();
       void newStatPool();
       int checkStatPool();

       inline bool isInit();

    private:
       StatMgrPool* _pool;
};
END_SPP_STAT_NS
#endif
