
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


#include "../comm/tbase/misc.h"
#include "defaultctrl.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

using namespace spp::ctrl;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("usage: ./spp_ctrl config_file\n");
        exit(-1);
    }

    mkdir("../log", 0700);
    mkdir("../stat", 0700);
    mkdir("../moni", 0700);
    CServerBase* ctrl = new CDefaultCtrl;
    ctrl->run(argc, argv);
    delete ctrl;
    
    return 0;
}
