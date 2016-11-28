
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


#ifndef __ISTAT_COST_H__
#define __ISTAT_COST_H__
#include "stddef.h"
namespace SPP_STAT_NS 
{
class ICostStat
{
    public:
        virtual ~ICostStat(){};
        /**
         * 初始化命令字统计, cmdid的范围为[0, 99], 只有初始化过的命令字才能对返回码进行统计.
         *
         * @param cmdid    命令字
         * @param cmddesc  命令字描述信息, 可为空, 上报不会把描述信息上报. 
         *
         * @return : 成功；=0：<0: 失败
         *
         */
        virtual int initStatItem(unsigned cmdid, const char* cmddesc = NULL) = 0;
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

        virtual int stepStatItem(const unsigned& cmdid,const unsigned& code,const unsigned& timecost, const char* codedesc = NULL) = 0;
        static ICostStat* instance();
};
}


/***********************************************
 * 用户使用应该调用如下跟函数对应的宏
 * ************************************************/
#define STEP_STAT_ITEM  SPP_STAT_NS::ICostStat::instance()->stepStatItem
#define INIT_STAT_ITEM  SPP_STAT_NS::ICostStat::instance()->initStatItem
#endif
