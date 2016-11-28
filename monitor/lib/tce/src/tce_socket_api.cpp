
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


#include "tce_socket_api.h"
#include <errno.h>
#include <stdio.h>

#ifdef WIN32
#pragma comment(lib,"Ws2_32.lib")
#endif

namespace tce{

int32_t socket_init()
{
#ifdef WIN32 
	WSADATA wsaData;
	if(WSAStartup(0x202,&wsaData))
	{
		return -1;
	}
#endif
	return 0;
}

int32_t socket_uninit()
{
#ifdef WIN32
	WSACleanup();
#endif
	return 0;
}

int32_t socket_close(const SOCKET iSocket)
{
#ifdef WIN32
	return ::closesocket(iSocket);
#else
	return ::close(iSocket);
#endif
}

int32_t socket_setNBlock(const SOCKET iSocket)
{
#ifdef WIN32
	uint32_t ul = 1;
	if(ioctlsocket(iSocket, FIONBIO, &ul) == SOCKET_ERROR)
		return -1;
	else
		return 0;
#else
	int32_t opts;
	opts=fcntl(iSocket,F_GETFL);
	if(opts<0)
	{
		printf("[error]**** fcntl(sock,GETFL), error=%s ****\n", strerror(errno));
		return -1;
	}
	opts = opts|O_NONBLOCK;
	if(fcntl(iSocket,F_SETFL,opts)<0)
	{
		printf("[error]**** fcntl(sock,SETFL,opts), error=%s ****\n", strerror(errno));
		return -1;
	}       
#endif
	return 0;
}

int32_t socket_setNCloseWait(const SOCKET iSocket, const int32_t iWaitSec)
{
	//for CLOSE_WAIT
	linger sLinger;
	sLinger.l_onoff = 1;  // (在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
	sLinger.l_linger = iWaitSec; // (容许逗留的时间为iWaitSec秒)
	if(setsockopt(iSocket, SOL_SOCKET,	SO_LINGER,	(char*)&sLinger, sizeof(linger)) != 0)
	{
//		printf("[error]**** err=%s, %s,%d ****\n", strerror(errno), __FILE__, __LINE__);
		return -1;
	}
	// 如果服务器终止后,服务器可以第二次快速启动而不用等待一段时间 
	int32_t nREUSEADDR = 1;
	if(setsockopt(iSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&nREUSEADDR, sizeof(int32_t)) != 0)
	{
//		printf("[error]**** err=%s, %s,%d ****\n", strerror(errno), __FILE__, __LINE__);
		return -1;
	}
	return 0;
}

int32_t socket_send(SOCKET sock, const char *buf, int32_t count)
{
	return ::send(sock, buf, count, 0);
}

};

