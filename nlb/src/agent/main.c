
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


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "commdef.h"
#include "config.h"
#include "agent.h"
#include "log.h"
#include "nlbfile.h"

static void sig_quit(int sig)
{
    NLOG_DEBUG("recevice quit signal...");
    set_quit();
}

/* agent进程单例检查 */
static bool singleton_check(void)
{
    int32_t ret;
    int32_t fd;
    const char *sing_file_lock = NLB_NAME_BASE_PATH"/.sigleton.lock";

    ret = mkdir_recursive(NLB_NAME_BASE_PATH);
    if (ret < 0) {
        printf("[ERROR] Make nlb agent data directory (%s) failed [%m]!!!\n", NLB_NAME_BASE_PATH);
        return false;
    }

    fd = open(sing_file_lock, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("[ERROR] Open file (%s) failed [%m]!!!\n", sing_file_lock);
        return false;
    }

    ret = flock(fd, LOCK_EX | LOCK_NB);
    if (ret < 0) {
        close(fd);
        return false;
    }

    /* 不关闭文件描述符 */

    return true;
}

static void daemon_init()
{
    /* 忽略必要信号 */
    signal(SIGHUP,  SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    /* 注册退出信号 */
    signal(SIGUSR1, sig_quit);

    /* 后台运行 */
    daemon(1, 1);
}

int main(int argc, char **argv)
{
    int32_t ret;

    parse_args(argc, argv);

    daemon_init();

    if (!singleton_check()) {
        printf("[ERROR] Agent singleton check failed, maybe repeated!!!\n");
        exit(1);
    }

    ret = init();
    if (ret < 0) {
        exit(1);
    }

    printf("Agent start success!!\n");

    while (true) {
        run();
    }

    return 0;
}

