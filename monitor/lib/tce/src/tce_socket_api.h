
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


#ifndef __TCE_SOCKET_H__
#define __TCE_SOCKET_H__


#ifdef WIN32
/*WIN32*/
#ifdef FD_SETSIZE
#pragma message ("")
#pragma message ("Please do not define FD_SETSIZE!")
#pragma message ("Please do not include Winsock.h!")
#pragma message ("Please do not include Winsock2.h!")
#pragma message ("")
#else

#define FD_SETSIZE 1024//MAX_SOCKET_NUM     //select的最大支持个数
#define WIN32_LEAN_AND_MEAN
#include <WINSOCK2.h>   //注意：包含Winsock2.h必须在定义FD_SETSIZE宏之后
#pragma message ("")
#pragma message ("define FD_SETSIZE!")
#pragma message ("include Winsock2.h!")
#pragma message ("")

#endif //FD_SETSIZE


//#undef FD_SET //Winsock2下的FD_SET效率太低了
//#define FD_SET(fd, set)  ((fd_set FAR *)(set))->fd_array[((fd_set FAR *)(set))->fd_count++] = (fd); 
#else    
/*LINUX*/
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <asm/unistd.h>

//suse
#include <sys/epoll.h>
//slackware
//#include <epoll.h>



#include <errno.h>

#ifndef __NR_epoll_create
#define __NR_epoll_create	254
#endif

#ifndef __NR_epoll_ctl
#define __NR_epoll_ctl		255
#endif

#ifndef __NR_epoll_wait
#define __NR_epoll_wait		256
#endif

//inline _syscall1(int,epoll_create,int,size);
//inline _syscall4(int,epoll_ctl,int,a,int, b, int, c, struct epoll_event*, d);
//inline _syscall4(int,epoll_wait,int,fd, struct epoll_event*, events, int,maxevents,int,timeout);


const int32_t MAX_CLIENT_COUNT = 50000;
const int32_t EPOLL_MAX_SIZE = 500*1024;

const int32_t EPOLL_EVENT_COUNT = 1024;
const int32_t EPOLL_WAIT_TIMEOUT = 0;//10;//2000; 
#endif

#ifdef WIN32
/* include */
#else
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<fcntl.h>
#include <string.h>
#endif

#ifdef WIN32
#define SOCKET SOCKET
#define socklen_t int
#else
#define SOCKET	int	
#define INVALID_SOCKET -1
#endif

#ifdef WIN32
#define ntohll(x)     _byteswap_uint64 (x)
#define htonll(x)     _byteswap_uint64 (x)
#else
#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x)       (x)
#define htonll(x)       (x)
#else 
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ntohll(x)     __bswap_64 (x)
#define htonll(x)     __bswap_64 (x)
#endif 
#endif  
#endif



namespace tce{

	int32_t socket_init();
	int32_t socket_uninit();
	int32_t socket_close(const SOCKET iSocket);
	int32_t socket_setNBlock(const SOCKET iSocket);
	int32_t socket_setNCloseWait(const SOCKET iSocket, const int32_t iWaitSec=0);
	int32_t socket_send(SOCKET sock, const char *buf, int32_t count);

};

#endif

