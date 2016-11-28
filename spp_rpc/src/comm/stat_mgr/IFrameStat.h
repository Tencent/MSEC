
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


#ifndef __IFRAME_STAT_H__
#define __IFRAME_STAT_H__
#include "stddef.h"

namespace SPP_STAT_NS 
{
class ICodeStat
{
    public:
        virtual ~ICodeStat(){};
        /**
         * 框架对于异步框架错误码统计 
         *
         * @param code     异步框架返回码 
         * @param cmddesc  命令字描述信息, 可为空, 上报不会把描述信息上报. 
         *
         * @return : 成功；=0：<0: 失败
         *
         */

        virtual int stepCodeStat(const unsigned& code, const char * desc = NULL) = 0;
        static ICodeStat* instance();
};

class IReqStat
{
    public:
        virtual ~IReqStat(){};
        /**
         * 框架对于前端请求时延统计
         *
         * @param timecost 时延信息, 单位毫秒. 
         *
         * @return : 成功；=0：<0: 失败
         *
         */


        virtual int stepReqStat(const unsigned& timecost) = 0;
        static IReqStat*  instance();
};


}
#define STEP_REQ_STAT SPP_STAT_NS::IReqStat::instance()->stepReqStat
#define STEP_CODE_STAT SPP_STAT_NS::ICodeStat::instance()->stepCodeStat
#endif
