
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


#ifndef _SPP_INCL_H_
#define _SPP_INCL_H_
#include "spp_incl/spp_version.h"	//框架版本号
#include "spp_incl/tlog.h"			//日志
#include "spp_incl/tstat.h"			//统计
#include "spp_incl/tcommu.h"		//通讯组件
#include "spp_incl/serverbase.h"	//服务器容器
#include "spp_incl/ICostStat.h"
#include "spp_incl/monitor.h"
#include "spp_incl/configini.h"

#define GROUPID(x) (((x)|1<<31))

using namespace tbase::tlog;
using namespace tbase::tstat;
using namespace tbase::tcommu;
using namespace spp::comm;
using namespace spp::comm;
using namespace SPP_STAT_NS;

#endif
