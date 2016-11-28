
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


/**
 * @filename utils.h
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })


/**
 * @brief 将所有数据写入fd
 */
int32_t write_all(int32_t fd, const char *buff, int32_t len);

/**
 * @brief 将所有数据写入文件
 */
int32_t write_all_2_file(const char *path, const char *data, int32_t len);

/**
 * @brief  通过网络接口名，获取接口IP地址
 * @info   默认采用eth0接口地址
 * @return =0   没有该接口或者接口ip地址为0.0.0.0
 *         其它 获取成功
 */
uint32_t get_ip(const char* intf);

/**
 * @brief  创建UDP套接字,并设置为非阻塞
 * @return >=0 返回套接字描述符 <0 错误
 */
int32_t create_udp_socket(void);

/**
 * @brief IP+port转换成Linux网络地址
 */
void make_inet_addr(const char *ip, uint16_t port, struct sockaddr_in *addr);

/**
 * @brief  网络套接字绑定端口
 * @return <0 失败 =0 成功
 */
int32_t bind_port(int32_t fd, const char *ip, uint16_t port);

/**
 * @brief  发送UDP包
 * @return =0 成功 <0 失败
 */
int32_t udp_send(int32_t fd, struct sockaddr_in *addr, const char *buff, int32_t len);

/**
 * @brief  接收UDP回包
 * @return >0 接收长度 <0 失败
 */
int32_t udp_recv(int32_t fd, char *buff, int32_t len, int32_t timeout);

#endif


