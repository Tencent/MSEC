
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


#ifndef _NLBFILE_H_
#define _NLBFILE_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include "commtype.h"
#include "commstruct.h"

/**
 * @brief 打开目录，如果目录不存在，需要先创建
 */
DIR *open_and_create_dir(const char *path);

/**
 * @brief 获取业务配置文件的目录
 */
int32_t get_service_dir(const char *name, char *path, int32_t len);

/**
 * @brief 获取Agent的域socket路径
 * @return 0 成功 <0 失败
 */
int32_t get_naming_agent_unix_path(char *path, int32_t len);

/**
 * @brief 获取业务配置目录
 */
int32_t get_naming_dir(const char *name, char *path, int32_t len);

/**
 * @brief 获取元数据文件路径
 */
int32_t get_naming_meta_path(const char *name, char *path, int32_t len);

/**
 * @brief 获取业务路由服务器存储文件路径
 * @return 0 成功 <0 失败
 */
int32_t get_naming_server_path(const char *name, uint32_t index, char *path, int32_t len);

/**
 * @brief 获取元数据文件锁路径
 * @return 0 成功 <0 失败
 */
int32_t get_naming_flock_pach(const char *name, char *path, int32_t len);

int32_t lock_meta(const char *name);

void unlock_meta(int32_t fd);

/**
 * @brief 返回meta文件长度，页框对齐
 */
uint32_t get_meta_file_size(void);


/**
 * @brief 返回路由服务器数据文件长度，按页框对齐
 */
uint32_t get_server_file_size(void);


/**
 * @brief 加载元数据到内存
 * @param name:   服务名
 *        mmaplen:mmap数据长度，unmap需要
 */ 
void *load_meta_data(const char *name, uint32_t *mmaplen);


/**
 * @brief 写入元数据到文件
 * @param meta: 元数据信息
 */ 
int32_t write_meta_data(const struct shm_meta *meta);


/**
 * @brief 初始化元数据文件，并load到内存
 * @param name:   服务名
 *        mmaplen:mmap数据长度，unmap需要
 */ 
void *init_and_load_meta_file(const char *name, uint32_t *mmaplen);


/**
 * @brief 加载路由服务器数据到内存
 * @param name:   服务名
 *        index:  当前数据索引
 *        mmaplen:mmap数据长度，unmap需要
 */ 
void *load_server_data(const char *name, uint32_t index, uint32_t *mmaplen);


/**
 * @brief 写服务器数据到文件
 * @param name:   服务名
 *        index:  当前数据索引
 *        mmaplen:mmap数据长度，unmap需要
 */ 
int32_t write_server_data(const char *name, uint32_t index, const struct shm_servers *servers);


/**
 * @brief 初始化并加载路由服务器数据到内存
 * @param name:   服务名
 *        index:  当前数据索引
 *        mmaplen:mmap数据长度，unmap需要
 */ 
void *init_and_load_server_data(const char *name, uint32_t index, uint32_t *mmaplen);

/**
 * @brief 递归创建目录
 */
int32_t mkdir_recursive(const char *absolute_path);

/**
 * @brief 检查指定目录是否存在
 */
bool check_dir_exist(const char* path);



#endif


