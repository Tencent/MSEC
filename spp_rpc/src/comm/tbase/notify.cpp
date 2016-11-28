
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


#include "notify.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>
#include <keygen.h>

using namespace tbase::notify;
using namespace spp::comm;

int g_spp_shm_fifo = false;

/**
 * 获取FIFO通知文件的路径，返回文件路径
  */
char *CNotify::get_fifo_path(int key)
{
    #define FIFO_SHM_DIR            "/dev/shm/spp_notify"

    int32_t ret = 0;
    static char fifo_path[128] = {0};

    /* 默认的fifo创建路径 */
    snprintf(fifo_path, sizeof(fifo_path), ".notify_%d", key);

    /* 启用fifo shm后，创建的文件在/dev/shm/spp/.notify_%s */
    if (g_spp_shm_fifo)
    {
        ret = mkdir(FIFO_SHM_DIR, 0777);
        if ((ret < 0) && (errno != EEXIST))
        {
            printf("create fifo shm directory failed, %s", strerror(errno));
            return fifo_path;
        }

        uint32_t pwd_key = pwdtok(key);

        snprintf(fifo_path, sizeof(fifo_path), FIFO_SHM_DIR"/.notify_0x%x_%u", pwd_key, (uint32_t)key);
    }

    return fifo_path;
}

int CNotify::notify_init(int key, char *fifoname, size_t fifoname_size)
{
	if(key <= 0) {
		return -1;
	}
	
	int netfd = -1;
	char *path = get_fifo_path(key);

	if( fifoname != NULL ) {
		strncpy( fifoname, path, fifoname_size-1 );
	}

	if(mkfifo(path, 0666) < 0) {
		if (errno != EEXIST) {
			printf("create fifo[%s] error[%m]\n", path);
			return -2;
		} 
	}

    netfd = open(path, O_RDWR | O_NONBLOCK, 0666);
    if(netfd == -1) {
        printf("open notify fifo[%s] error[%m]\n", path);
        return -3;
    }
	//已经在serverbase.cpp中忽略此信号
	//signal(SIGPIPE, SIG_IGN);	

	return netfd;
}

int CNotify::notify_send(int fd)
{
	return write(fd, "x", 1);
}

int CNotify::notify_recv(int fd)
{
	char buf[64];
	return read(fd, buf, sizeof(buf));
}


