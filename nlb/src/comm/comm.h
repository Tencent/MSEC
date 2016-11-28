
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
 * @filename comm.h
 */
#ifndef _COMM_H_
#define _COMM_H_

#include <stdint.h>
#include "commstruct.h"

/**
 * @brief 初始化shm_servers
 */
void init_shm_servers(struct shm_servers *servers);

/**
 * @brief 通过IP查找路由服务器信息
 */
struct server_info *get_server_by_ip(struct shm_servers *svrs, uint32_t ip);


#endif

