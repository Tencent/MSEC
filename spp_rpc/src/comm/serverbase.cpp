
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


#include "serverbase.h"
#include "global.h"
#include "spp_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

using namespace spp::comm;
using namespace spp::global;

void ShowVer(void)
{
    printf("  srpc version: %d.%d.%d\n", SPP_MAJOR_VER, SPP_MINOR_VER, SPP_PATCH_VER);
    printf("  mt   version: %s\n", MT_VERSION);
    printf("  Date: %s\n", __DATE__);
}

unsigned char CServerBase::flag_ = 0;

CServerBase::CServerBase()
{
    ix_ = new TInternal;
    ix_->argc_ = 0;
    ix_->argv_ = NULL;
    ix_->moni_inter_time_ = 15;
}

CServerBase::~CServerBase()
{
    if (ix_ != NULL)
    {
        if (ix_->argv_ != NULL)
        {
            for (unsigned i = 0; i < (unsigned)ix_->argc_; ++i)
                if (ix_->argv_[i] != NULL)
                    free( ix_->argv_[i] );

            delete [] ix_->argv_;
        }

        delete ix_;
    }
}

void CServerBase::run(int argc, char* argv[])
{
    if (argc < 2)
    {
        ShowVer();
		
        printf("usage: %s config_file\n\n", argv[0]);
        return;
    }

    char* p   = strstr(argv[1], "-");

    if (p != NULL)
    {
        ++p;

        if (*p == '-')
            ++p;

        if (*p == 'v' && *(p + 1) == '\0')
        {
            ShowVer();
        }
        else if (*p == 'h' && *(p + 1) == '\0')
        {
            ShowVer();

            printf("usage: %s config_file\n\n", argv[0]);
        }
        else
        {
            printf("\ninvalid argument.\n");

            ShowVer();

            printf("usage: %s config_file\n\n", argv[0]);
        }

        return;
    }

    //拷贝参数
    spp_global::save_arg(argc, argv);
    ix_->argc_ = argc;
    ix_->argv_ = new char*[ix_->argc_ + 1];
    int i;

    for (i = 0; i < ix_->argc_; ++i)
        ix_->argv_[i] = strdup(argv[i]);

    ix_->argv_[ix_->argc_] = NULL;

    startup(true);

    realrun(argc, argv);
}

void CServerBase::startup(bool bg_run)
{
    //默认需要root权限才能setrlimit
    struct rlimit rlim;
    if (0 == getrlimit(RLIMIT_NOFILE, &rlim))
    {
        rlim.rlim_cur = rlim.rlim_max;
        setrlimit(RLIMIT_NOFILE, &rlim);
        if (rlim.rlim_cur < 100000)     // fix for limits over 100000
        {
            rlim.rlim_cur = 100000;
            rlim.rlim_max = 100000;
            setrlimit(RLIMIT_NOFILE, &rlim);
        }
    }

    mallopt(M_MMAP_THRESHOLD, 1024*1024);   // 1MB，防止频繁mmap
    mallopt(M_TRIM_THRESHOLD, 8*1024*1024); // 8MB，防止频繁brk

    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

    if (bg_run)
    {
        signal(SIGINT,  SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        daemon(1, 1);
    }

    CServerBase::flag_ = 0;
    //signal(SIGSEGV, CServerBase::sigsegv_handle);
    signal(SIGUSR1, CServerBase::sigusr1_handle);
    signal(SIGUSR2, CServerBase::sigusr2_handle);
}
bool CServerBase::reload()
{
    if (flag_ & RUN_FLAG_RELOAD)
    {
        flag_ &= ~RUN_FLAG_RELOAD;
        return true;
    }
    else
        return false;
}
bool CServerBase::quit()
{
    if (flag_ & RUN_FLAG_QUIT)
        return true;
    else
        return false;
}
void CServerBase::sigusr1_handle(int signo)
{
    flag_ |= RUN_FLAG_QUIT;
    signal(SIGUSR1, CServerBase::sigusr1_handle);
}
void CServerBase::sigusr2_handle(int signo)
{
    flag_ |= RUN_FLAG_RELOAD;
    signal(SIGUSR2, CServerBase::sigusr2_handle);
}

