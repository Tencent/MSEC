
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
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <zookeeper.h>
#include "log.h"
#include "utils.h"
#include "commdef.h"
#include "commtype.h"
#include "networking.h"
#include "config.h"
#include "nlbtime.h"
#include "agent.h"
#include "zkheartbeat.h"

static zhandle_t *zh;
static clientid_t myid;
static const char *client_id_file = NLB_NAME_BASE_PATH"/.zk_client_id";
static int32_t restart_flag = false;
static time_t  last_restart_time;
//static FILE *log_fp;

/* zookeeper事件类型转换成字符串 */
const char* zk_type_2_str(int32_t type)
{
    if (type == ZOO_CREATED_EVENT)
        return "CREATED_EVENT";
    if (type == ZOO_DELETED_EVENT)
        return "DELETED_EVENT";
    if (type == ZOO_CHANGED_EVENT)
        return "CHANGED_EVENT";
    if (type == ZOO_CHILD_EVENT)
        return "CHILD_EVENT";
    if (type == ZOO_SESSION_EVENT)
        return "SESSION_EVENT";
    if (type == ZOO_NOTWATCHING_EVENT)
        return "NOTWATCHING_EVENT";

    return "UNKNOWN_EVENT_TYPE";
}

/* zookeeper客户端状态转换成字符串 */
const char* zk_stat_2_str(int32_t state)
{
    if (state == 0)
        return "CLOSED_STATE";
    if (state == ZOO_CONNECTING_STATE)
        return "CONNECTING_STATE";
    if (state == ZOO_ASSOCIATING_STATE)
        return "ASSOCIATING_STATE";
    if (state == ZOO_CONNECTED_STATE)
        return "CONNECTED_STATE";
    if (state == ZOO_EXPIRED_SESSION_STATE)
        return "EXPIRED_SESSION_STATE";
    if (state == ZOO_AUTH_FAILED_STATE)
        return "AUTH_FAILED_STATE";
  
    return "INVALID_STATE";
}

/* 检查zookeeper是否已经连接上 */
bool zk_connected(void)
{
    return zoo_state(zh) == ZOO_CONNECTED_STATE;
}

/* 检查是否需要重新初始化zookeeper */
bool zk_need_reinit(void)
{
    if (last_restart_time == time(NULL)) {
        return false;
    }

    if (is_unrecoverable(zh)) {
        return true;
    }

    if (restart_flag) {
        restart_flag = false;
        return true;
    }

    return false;
}

/* zookeeper写clientid到id文件 */
static void zk_write_client_id(const char *path, const clientid_t *id)
{
    int32_t ret;
    int32_t fd;

    if (NULL == path || NULL == id) {
        NLOG_DEBUG("No zk client_id_file");
        return;
    }

    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        NLOG_ERROR("Open zk client_id file (%s) failed, [%m]", path);
        return;
    }

    ret = write_all(fd, (char *)id, sizeof(*id));
    if (ret < 0) {
        close(fd);
        NLOG_ERROR("Write zk client_id failed, ret [%d]", ret);
        return;
    }

    close(fd);
}

/**
 * @brief zookeeper创建完成回调函数
 */
static void simple_create_complate(int32_t rc, const char *name, const void *data)
{
    if ((rc == ZNODEEXISTS) || (rc == ZOK)) {
        NLOG_INFO("create node (%s) success", (char *)data);
        free((void *)data);
        return;
    }

    NLOG_ERROR("create node (%s) failed, [%s]", (char *)data, zerror(rc));
    free((void *)data);
}

/**
 * @brief  zookeeper创建节点函数
 * @return =0 成功 <0 失败
 */
int32_t zk_simple_create(const char *path)
{
    int32_t ret;
    char *  data;

    if (NULL == path) {
        NLOG_ERROR("Invalid path for create");
        return -1;
    }

    data = strdup(path);
    if (NULL == data) {
        NLOG_ERROR("No memory");
        return -2;
    }

    ret  = zoo_acreate(zh, path, NULL, 0, &ZOO_OPEN_ACL_UNSAFE,
                       0, simple_create_complate, data);
    if (ret != ZOK) {
        NLOG_ERROR("create node (%s) failed, [%s]", path, zerror(ret));
        free(data);
        return -3;
    }

    return 0;
}

/* 获取zookeeper客户端句柄 */
zhandle_t *get_zk_instance(void)
{
    return zh;
}

/* zookeeper全局session监听回调函数 */
static void zk_watch_global(zhandle_t *zzh, int type, int state, const char *path, void* context)
{
    NLOG_DEBUG("Zookeeper global watcher %s state = %s", zk_type_2_str(type), zk_stat_2_str(state));

    if (type == ZOO_SESSION_EVENT) {
        if (state == ZOO_CONNECTED_STATE) {
            const clientid_t *id = zoo_client_id(zzh);
            if (myid.client_id == 0 || myid.client_id != id->client_id) {
                myid = *id;
                NLOG_DEBUG("Zookeeper got a new session id: 0x%llx", (long long)myid.client_id);
                zk_write_client_id(client_id_file, id);
                return;
            }
        } else if (state == ZOO_AUTH_FAILED_STATE) {
            restart_flag = true;
            NLOG_ERROR("Zookeeper authentication failure. Restart...");
            return;
        } else if (state == ZOO_EXPIRED_SESSION_STATE) {
            restart_flag = true;
            NLOG_ERROR("Zookeeper session expired. Restart...");
            return;
        }
    }
}

/**
 * @brief zookeeper客户端句柄初始化
 */
int32_t nlb_zk_init(const char *host, int32_t timeout)
{
#if 0
    log_fp = fopen("../log/nlb.log", "a+");
    if (NULL == log_fp) {
        NLOG_ERROR("Open log file failed.");
        return -1;
    }
#endif

    setLogLevel(get_log_level());
    memset(&myid, 0, sizeof(myid));
    zh = zookeeper_init(host, zk_watch_global, timeout, &myid, 0, 0);
    if (!zh) {
        NLOG_ERROR("Zookeeper_init failed, host:[%s] timeout:[%d] [%m].", get_zk_host(), timeout);
        return -1;
    }

    return 0;
}

/**
 * @brief 获取zookeeper的poll事件
 */
int32_t nlb_zk_poll_events(int32_t *fd, int32_t *nlb_events, uint64_t *timeout)
{
    int32_t ret;
    int32_t zkfd, interest = 0;
    int32_t events = 0;
    struct timeval tv;

    if (NULL == fd || NULL == nlb_events || NULL == timeout) {
        NLOG_ERROR("Invalid input parameter.");
        return -1;
    }

    ret = zookeeper_interest(zh, &zkfd, &interest, &tv);
    if (ret != ZOK) {
        NLOG_ERROR("zookeeper_interest failed, err [%s]", zerror(ret));
        return -2;
    }

    if (zkfd != -1)
    {
        if (interest & ZOOKEEPER_READ) {
            events |= NLB_POLLIN;
        }

        if (interest & ZOOKEEPER_WRITE) {
            events |= NLB_POLLOUT;
        }
    }

    *fd         = zkfd;
    *nlb_events = events;
    *timeout    = covert_tv_2_ms(&tv);

    return 0;
}

/**
 * @brief zookeeper主处理函数
 */
void nlb_zk_process(uint32_t nlb_events)
{
    int32_t ret;
    int32_t zk_events = 0;
    if (nlb_events & NLB_POLLIN) {
        zk_events |= ZOOKEEPER_READ;
    }

    if (nlb_events & NLB_POLLOUT) {
        zk_events |= ZOOKEEPER_WRITE;
    }

    ret = zookeeper_process(zh, zk_events);
    if ((ret != ZOK) && (ret != ZNOTHING)) {
        NLOG_INFO("zookeeper_process failed, err [%s]", zerror(ret));
    }

    if (ret == ZCONNECTIONLOSS
        || ret == ZSESSIONEXPIRED
        || ret == ZINVALIDSTATE
        || ret == ZAUTHFAILED) {
        restart_flag = true;
    }
}

/**
 * @brief 关闭zookeeper句柄
 */
void nlb_zk_close(void)
{
    zookeeper_close(zh);
    zh = NULL;

#if 0
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
#endif
}

/**
 * @brief 重新初始化zookeeper句柄
 */
int32_t nlb_zk_reinit(void)
{
    int32_t ret;

    last_restart_time = time(NULL);

    NLOG_ERROR("Nlb reinit!!!!!!");

    nlb_zk_close();

    clean_nodes_watching();
    clean_services_watching();

    ret = nlb_zk_init(get_zk_host(), get_zk_timeout());
    if (ret < 0) {
        NLOG_ERROR("Nlb zookeeper init failed, ret [%d]", ret);
        return -1;
    }

    return 0;
}


