
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
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "commdef.h"
#include "commtype.h"
#include "commstruct.h"
#include "utils.h"
#include "nlbfile.h"

/**
 * @brief 打开目录，如果目录不存在，需要先创建
 */
DIR *open_and_create_dir(const char *path)
{
    int32_t ret;
    DIR *dir;

    if (NULL == path || path[0] != '/') {
        return NULL;
    }

    dir = opendir(path);
    if (dir) {
        return dir;
    } else if (errno != ENOENT) {
        return NULL;
    }

    ret = mkdir_recursive(path);
    if (ret < 0) {
        return NULL;
    }

    return opendir(path);
}


/**
 * @brief 获取Agent的域socket路径
 * @return 0 成功 <0 失败
 */
int32_t get_naming_agent_unix_path(char *path, int32_t len)
{
    int32_t rlen;

    if (NULL == path || len < 32) {
        return -1;
    }

    rlen = snprintf(path, len, "%s", NLB_NAME_BASE_PATH"/.agent_unix");
    if (rlen >= len) {
        return -2;
    }

    return 0;
}

/**
 * @brief 获取业务配置目录
 */
int32_t get_naming_dir(const char *name, char *path, int32_t len)
{
    int32_t rlen;
    char *  pos;

    if (NULL == path || len < 32) {
        return -1;
    }

    rlen = snprintf(path, len, "%s/%s", NLB_NAME_BASE_PATH, name);
    if (rlen >= len) {
        return -2;
    }

    pos = strchr(path, '.');
    if (NULL == pos) {
        return -3;
    }

    *pos = '/';

    return 0;
}

/**
 * @brief 获取业务配置文件的目录
 */
int32_t get_service_dir(const char *name, char *path, int32_t len)
{
    int32_t rlen;
    char *  pos;

    if (NULL == path || len < 32) {
        return -1;
    }

    rlen = snprintf(path, len, "%s/%s", NLB_NAME_BASE_PATH, name);
    if (rlen >= len) {
        return -2;
    }

    pos = strchr(path, '.');
    if (NULL == pos) {
        return -3;
    }

    *pos = '/';

    return 0;
}

/**
 * @brief 获取元数据文件路径
 */
int32_t get_naming_meta_path(const char *name, char *path, int32_t len)
{
    int32_t rlen;
    char *  pos;

    if (NULL == path || len < 32) {
        return -1;
    }

    rlen = snprintf(path, len, "%s/%s/%s", NLB_NAME_BASE_PATH, name, "meta.dat");
    if (rlen >= len) {
        return -2;
    }

    pos = strchr(path, '.');
    if (NULL == pos) {
        return -3;
    }

    *pos = '/';

    return 0;
}

/**
 * @brief 获取业务路由服务器存储文件路径
 * @return 0 成功 <0 失败
 */
int32_t get_naming_server_path(const char *name, uint32_t index, char *path, int32_t len)
{
    int32_t rlen;
    char *  pos;

    if (NULL == path || len < 32) {
        return -1;
    }

    rlen = snprintf(path, len, "%s/%s/%s%u", NLB_NAME_BASE_PATH, name, "servers.dat", index);
    if (rlen >= len) {
        return -2;
    }

    pos = strchr(path, '.');
    if (NULL == pos) {
        return -3;
    }

    *pos = '/';

    return 0;
}

/**
 * @brief 获取元数据文件锁路径
 * @return 0 成功 <0 失败
 */
int32_t get_naming_flock_pach(const char *name, char *path, int32_t len)
{
    int32_t rlen;
    char *  pos;

    if (NULL == path || len < 32) {
        return -1;
    }

    rlen = snprintf(path, len, "%s/%s/%s", NLB_NAME_BASE_PATH, name, "meta.lock");
    if (rlen >= len) {
        return -2;
    }

    pos = strchr(path, '.');
    if (NULL == pos) {
        return -3;
    }

    *pos = '/';

    return 0;
}

int32_t lock_meta(const char *name)
{
    int32_t ret;
    int32_t fd;
    char    path[NLB_PATH_MAX_LEN];

    get_naming_flock_pach(name, path, NLB_PATH_MAX_LEN);
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        return -1;
    }

    ret = flock(fd, LOCK_EX | LOCK_NB);
    if (ret < 0) {
        close(fd);
        return -2;
    }

    return fd;
}

void unlock_meta(int32_t fd)
{
    flock(fd, LOCK_UN);
    close(fd);
}

/**
 * @brief 返回meta文件长度，页框对齐
 */
uint32_t get_meta_file_size(void)
{
    uint32_t page_size = sysconf(_SC_PAGE_SIZE);
    uint32_t meta_len  = sizeof(struct shm_meta);
    uint32_t real_len;

    real_len = (meta_len + page_size - 1)/page_size*page_size;

    return real_len;
}

/**
 * @brief 返回路由服务器数据文件长度，按页框对齐
 */
uint32_t get_server_file_size(void)
{
    uint32_t page_size = sysconf(_SC_PAGE_SIZE);
    uint32_t data_len  = (sizeof(struct shm_servers) + sizeof(struct server_info) * NLB_SERVER_MAX);
    uint32_t real_len;

    real_len = (data_len + page_size - 1)/page_size*page_size;

    return real_len;
}

/**
 * @brief 加载元数据到内存
 * @param name:   服务名
 *        mmaplen:mmap数据长度，unmap需要
 */ 
void *load_meta_data(const char *name, uint32_t *mmaplen)
{
    int32_t  lock_fd = -1;
    int32_t  fd = -1;
    int32_t  ret;
    char     path[256];
    void *   addr;
    struct stat buf;

    /* 先锁住meta，防止和API产生冲突 */
    lock_fd = lock_meta(name);
    if (lock_fd < 0) {
        goto ERR_RET;
    }

    /* 获取服务器数据文件路径 */
    ret = get_naming_meta_path(name, path, 256);
    if (ret < 0) {
        goto ERR_RET;
    }

    fd = open(path, O_RDWR);
    if (fd == -1) {
        goto ERR_RET;
    }

    /* 检查文件有效性 */
    ret = fstat(fd, &buf);
    if (ret == -1) {
        goto ERR_RET;
    }

    if (buf.st_size != get_meta_file_size()) {
        goto ERR_RET;
    }

    /* 加载数据到内存 */
    addr = mmap(NULL, buf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        goto ERR_RET;
    }

    close(fd);
    *mmaplen = buf.st_size;
    unlock_meta(lock_fd);
    return addr;

ERR_RET:
    if (fd >= 0) {
        close(fd);
    }

    if (lock_fd >= 0) {
        unlock_meta(lock_fd);
    }

    return NULL;
}

/**
 * @brief 写入元数据到文件
 * @param meta: 元数据信息
 */ 
int32_t write_meta_data(const struct shm_meta *meta)
{
    int32_t  lock_fd = -1;
    int32_t  ret, result = 0;
    uint32_t size;
    char     path[256];

    /* 先锁住meta */
    lock_fd = lock_meta(meta->name);
    if (lock_fd < 0) {
        result = -1;
        goto RET;
    }

    /* 获取服务器数据文件路径 */
    ret = get_naming_meta_path(meta->name, path, NLB_SERVICE_NAME_LEN);
    if (ret < 0) {
        result = -2;
        goto RET;
    }

    /* 写数据到文件 */
    ret = write_all_2_file(path, (char *)meta, sizeof(*meta));
    if (ret < 0) {
        result = -3;
        goto RET;
    }

    /* 设置文件大小 */
    size = get_meta_file_size();
    ret  = truncate(path, size);
    if (ret == -1) {
        result = -4;
        goto RET;
    }

RET:
    if (lock_fd >= 0) {
        unlock_meta(lock_fd);
    }

    return result;
}


/**
 * @brief 初始化元数据文件，并load到内存
 * @param name:   服务名
 *        mmaplen:mmap数据长度，unmap需要
 */ 
void *init_and_load_meta_file(const char *name, uint32_t *mmaplen)
{
    int32_t  fd = -1;
    int32_t  ret;
    uint32_t size;
    char     path[256];
    void *   addr;

    /* 获取服务器数据文件路径 */
    ret = get_naming_meta_path(name, path, 256);
    if (ret < 0) {
        goto ERR_RET;
    }

    /* 获取并设置meta文件大小 */
    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        goto ERR_RET;
    }

    size = get_meta_file_size();
    ret = ftruncate(fd, size);
    if (ret == -1) {
        goto ERR_RET;
    }

    /* 加载数据到内存 */
    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        goto ERR_RET;
    }

    close(fd);
    *mmaplen = size;
    return addr;

ERR_RET:
    if (fd >= 0) {
        close(fd);
    }

    return NULL;
}



/**
 * @brief 加载路由服务器数据到内存
 * @param name:   服务名
 *        index:  当前数据索引
 *        mmaplen:mmap数据长度，unmap需要
 */ 
void *load_server_data(const char *name, uint32_t index, uint32_t *mmaplen)
{
    int32_t  fd = -1;
    int32_t  ret;
    char     path[256];
    void *   addr;
    struct stat buf;

    /* 获取服务器数据文件路径 */
    ret = get_naming_server_path(name, index, path, 256);
    if (ret < 0) {
        goto ERR_RET;
    }

    fd = open(path, O_RDWR);
    if (fd == -1) {
        goto ERR_RET;
    }

    /* 检查文件有效性 */
    ret = fstat(fd, &buf);
    if (ret == -1) {
        goto ERR_RET;
    }

    if (buf.st_size != get_server_file_size()) {
        goto ERR_RET;
    }

    /* 加载数据到内存 */
    addr = mmap(NULL, buf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        goto ERR_RET;
    }

    close(fd);
    *mmaplen = buf.st_size;
    return addr;

ERR_RET:
    if (fd >= 0) {
        close(fd);
    }

    return NULL;
}

/**
 * @brief 写服务器数据到文件
 * @param name:   服务名
 *        index:  当前数据索引
 *        mmaplen:mmap数据长度，unmap需要
 */ 
int32_t write_server_data(const char *name, uint32_t index, const struct shm_servers *servers)
{
    int32_t  ret;
    uint32_t size;
    uint32_t data_len;
    char     path[NLB_PATH_MAX_LEN];

    /* 获取服务器数据文件路径 */
    ret = get_naming_server_path(name, index, path, 256);
    if (ret < 0) {
        return -1;
    }

    /* 写数据到文件 */
    data_len = sizeof(struct shm_servers) + servers->server_num * sizeof(struct server_info);
    ret = write_all_2_file(path, (char *)servers, data_len);
    if (ret < 0) {
        return -2;
    }

    /* 设置文件大小 */
    size = get_server_file_size();
    ret  = truncate(path, size);
    if (ret == -1) {
        return -3;
    }

    return 0;
}


/**
 * @brief 初始化并加载路由服务器数据到内存
 * @param name:   服务名
 *        index:  当前数据索引
 *        mmaplen:mmap数据长度，unmap需要
 */ 
void *init_and_load_server_data(const char *name, uint32_t index, uint32_t *mmaplen)
{
    int32_t  fd = -1;
    int32_t  ret;
    uint32_t size;
    char     path[256];
    void *   addr;

    /* 获取服务器数据文件路径 */
    ret = get_naming_server_path(name, index, path, 256);
    if (ret < 0) {
        goto ERR_RET;
    }

    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        goto ERR_RET;
    }

    size = get_server_file_size();
    ret = ftruncate(fd, size);
    if (ret == -1) {
        goto ERR_RET;
    }

    /* 加载数据到内存 */
    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        goto ERR_RET;
    }

    close(fd);
    *mmaplen = size;
    return addr;

ERR_RET:
    if (fd >= 0) {
        close(fd);
    }

    return NULL;
}

/**
 * @brief 检查指定目录是否存在
 */
bool check_dir_exist(const char* path)
{
    if (!path) {
        return false;
    }

    DIR* dir = opendir(path);
    if (!dir) {
        return false;
    } else {
        closedir(dir);
        return true;
    } 
}



/**
 * @brief 检查并创建目录
 */
bool check_and_mkdir(const char *path)
{
    if (!path) {
        return false;
    }

    if (!check_dir_exist(path)) {
        if (mkdir(path, 0666) < 0)  {
            return false;
        }
    }

    return true;
}



/**
 * @brief 递归创建目录
 */
int32_t mkdir_recursive(const char *absolute_path)
{
    int32_t len;
    const char *pos;
    char    path[NLB_PATH_MAX_LEN];

    if ((NULL == absolute_path) || (absolute_path[0] != '/')) {
        return -1;
    }

    /* 循环创建目录 */
    pos = absolute_path + 1;
    while (NULL != (pos = index(pos, '/'))) {
        len = pos - absolute_path;

        if (len >= NLB_PATH_MAX_LEN) {
            return -2;
        }

        strncpy(path, absolute_path, len);
        *(path + len) = '\0';
        
        if (!check_and_mkdir(path)) {
            return -3;
        }

        pos++;
    }

    /* 检查并创建目录 */
    if (!check_and_mkdir(absolute_path)) {
        return -4;
    }

    return 0;
}




