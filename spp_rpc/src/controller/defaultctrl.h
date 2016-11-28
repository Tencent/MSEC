
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


#ifndef _SPP_CTRL_DEFAULT_H_
#define _SPP_CTRL_DEFAULT_H_
#include<string>
#include<vector>
#include <map>
#include "../comm/frame.h"
#include "../comm/serverbase.h"
#include"../comm/comm_def.h"
#include "../comm/tbase/hide_private_tp.h"
#include "../comm/tbase/tcommu.h"
#include "../comm/tbase/tshmcommu.h"
#include "../comm/tbase/hide_private_tp.h"
#include "StatComDef.h"
using namespace::std;
using namespace spp::comm;
using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;

namespace spp
{
    namespace ctrl
    {
        class CDefaultCtrl : public CServerBase, public CFrame
        {
        public:
            CDefaultCtrl();
            ~CDefaultCtrl();

            //实际运行函数
            void realrun(int argc, char* argv[]);
            //服务容器类型
            int servertype() {
                return SERVER_TYPE_CTRL;
            }

            void dump_pid();
            void check_ctrl_running();
            void clear_stat_file();

            //初始化配置
            int initconf(bool reload = false);
            int reloadconf();
            int initmap(bool reload);
            int loop();

        protected:
            //监控srv
            CTProcMonSrv monsrv_;
            // 输出pid文件fd
            int pidfile_fd_;
            // 退出时，执行的指令
            vector<string> cmd_on_exit_;

        };
    }
}
#endif
