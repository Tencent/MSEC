
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


#include "misc.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "atomic.h"
#include <time.h>
#include <signal.h>

using namespace spp::comm;

// if ifstr == NULL; use eth1(inner network)
// if ifstr == "all"; use 0.0.0.0
// else query the interface IP using ifstr
unsigned CMisc::getip(const char* ifstr)
{
    char ifname[16] = {0};
    if (!ifstr)
    {
        strncpy(ifname, "eth1", sizeof(ifname) - 1);
    }
    else if(0 == strncmp(ifname, "all", sizeof(ifname)))
    {
        return 0U; // all interface = IP 0.0.0.0
    }
    else
    {
        strncpy(ifname, ifstr, sizeof(ifname) -1);
    }

    register int fd, intrface;
    struct ifreq buf[256];
    struct ifconf ifc;
    unsigned ip = 0;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t)buf;

        if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
        {
            intrface = ifc.ifc_len / sizeof(struct ifreq);

            while (intrface-- > 0)
            {
                if (strcmp(buf[intrface].ifr_name, ifname) == 0)
                {
                    if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface])))
                        ip = (unsigned)((struct sockaddr_in *)(&buf[intrface].ifr_addr))->sin_addr.s_addr;

                    break;
                }
            }
        }

        close(fd);
    }

    return ip;
}

int CMisc::check_process_exist(pid_t pid)
{
    int ret = kill(pid, 0);

    if (ret == 0 || errno != ESRCH)
    {
        // 存在
        return 1;
    }

    // 不存在
    return 0;
}


int64_t CMisc::get_file_mtime(const char *path)
{
    struct stat buf;

    if (NULL == path || path[0] == '\0') {
        return -1;
    }

    if (lstat(path, &buf) == -1) {
        return -1;
    }

    return (int64_t)buf.st_mtime;
}
