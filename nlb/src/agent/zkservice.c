
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


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commdef.h"
#include "zkservice.h"
#include "agent.h"
#include "zookeeper.h"
#include "zkplugin.h"
#include "log.h"
#include "config.h"
#include "event.h"
#include "routeprocess.h"
#include "jsonparser.h"

/**
 * @brief 获取zookeeper节点路径
 */
static int32_t make_zk_service_path(const char *name, char *buff, int32_t len)
{
    int32_t slen;
    char *pos;

    slen = snprintf(buff, len, "/nameservice/%s", name);
    if (slen >= len) {
        return -1;
    }

    pos = strchr(buff, '.');
    if (NULL == pos) {
        return -2;
    }

    *pos = '/';

    return 0;
}

/**
 * @brief 清除service监视标记
 */
static void clean_service_watching(const char *name)
{
    struct agent_local_rdata *rdata;

    rdata = get_local_rdata(name);
    if (rdata) {
        rdata->watcher_flag = false;
    }
}

/**
 * @brief 设置service监视标记
 */
static void set_service_watching(const char *name)
{
    struct agent_local_rdata *rdata;

    rdata = get_local_rdata(name);
    if (rdata) {
        rdata->watcher_flag = true;
    }
}

/**
 * @brief 设置service监视标记
 */
static bool is_service_watching(const char *name)
{
    struct agent_local_rdata *rdata;

    rdata = get_local_rdata(name);
    if (rdata) {
        return rdata->watcher_flag;
    }

    return false;
}


/**
 * @brief 业务配置exists完成回调函数
 */
static void nameservice_exists_completion(int32_t rc, const struct Stat *stat, const void *data)
{
    struct agent_local_rdata *rdata = (struct agent_local_rdata *)data;
    char *name = rdata->name;

    /* 没有该节点 */
    if (rc == ZNONODE) {  // TODO: 是否需要关注业务不存在的情况??
        NLOG_DEBUG("service_exists_completion not found service (%s) node", name);
        set_service_watching(name);
        return ;
    }

    /* zookeeper错误码 */
    if (rc != ZOK) {
        clean_service_watching(name);
        NLOG_ERROR("service_exists_completion failed, [%s]", zerror(rc));
        return;
    }

    set_service_watching(name);

    /* 如果和本地数据一致，不变化 */
    if (rdata->route_meta->mtime == (uint64_t)stat->mtime) {
        return;
    }

    /* 如果数据变化了，事件通知，去拉取最新数据 */
    add_service_event(name);
}

/**
 * @brief zookeeper nameservice目录exists监视回调函数
 * @info  监控业务名目录(/nameservice/msec/monitor),如果发生ZOO_CREATED_EVENT | ZOO_CHANGED_EVENT，重新拉取业务配置
 */
static void nameservice_exists_watcher(zhandle_t *zzh, int32_t type, int32_t state, const char *path, void *context)
{
    char *name = ((struct agent_local_rdata *)context)->name;

    NLOG_DEBUG("watcher %s state = %s", zk_type_2_str(type), zk_stat_2_str(state));

    if (state == ZOO_CONNECTED_STATE) {
        if (state == ZOO_SESSION_EVENT) {
            return;
        }

        if (type == ZOO_CREATED_EVENT || type == ZOO_CHANGED_EVENT) {
            NLOG_INFO("recevied service %s changed event ", name);
            add_service_event(name);
        }

        if (type == ZOO_DELETED_EVENT) {
            NLOG_ERROR("recevied service %s delete event", name);
        }
    }

    clean_service_watching(name);
}

/**
 * @brief 设置业务监视事件
 * @info  必须保证在本地有该业务
 */
void set_service_watcher(struct agent_local_rdata *rdata)
{
    int32_t ret;
    char path[NLB_PATH_MAX_LEN];

    if (!zk_connected() || is_service_watching(rdata->name)) {
        return;
    }

    /* 设置业务监控事件 */
    make_zk_service_path(rdata->name, path, sizeof(path));
    ret = zoo_awexists(get_zk_instance(), path, nameservice_exists_watcher, rdata, nameservice_exists_completion, rdata);
    if (ret != ZOK) {
        NLOG_ERROR("zoo_awexists (%s) failed, [%s]", path, zerror(ret));
    }

    return;
}

/**
 * @brief  处理从zookeeper新加载的配置
 * @return =0 成功 <0 失败
 */
int32_t process_load_config(const char *name, const char *value, int32_t value_len, uint64_t mtime)
{
    int32_t ret;
    struct shm_servers *servers = NULL;
    struct agent_local_rdata *rdata;

    /* 解析json协议 */
    ret = json_parse_service(value, value_len, &servers);
    if (ret < 0) {
        NLOG_ERROR("Parse nameservice (%s) json config failed, ret [%d].", name, ret);
        return -1;
    }

    /* 如果本地有该业务路由数据，只需要更新 */ 
    rdata = get_local_rdata(name);
    if (rdata != NULL) {
        update_config(rdata, servers, mtime);
        free(servers);
        return 0;
    }

    /* 如果本地没有该业务路由数据，需要重新创建 */
    ret = add_new_service(name, servers, mtime);
    if (ret < 0) {
        NLOG_ERROR("add new service failed, ret [%d]", ret);
        free(servers);
        return -2;
    }

    free(servers);

    return 0;
}

/**
 * @brief  获取业务配置信息回调函数
 * @info   需要区分新业务请求和NLB_EVENT_TYPE_LOAD_SERVICE事件两种情况
 * @return =0 成功 <0 失败
 */
static void nameservice_aget_completion(int32_t rc, const char *value, int32_t value_len, 
                                 const struct Stat *stat, const void *data)
{
    int32_t ret;
    struct agent_local_rdata *rdata;

    /* 没有该业务 */
    if (rc == ZNONODE) {
        NLOG_DEBUG("get service (%s) route config failed, [%s]", (char *)data, zerror(rc));
        goto ERR_RET;
    }

    /* zookeeper自身错误 */
    if (rc != ZOK) {
        NLOG_ERROR("get service (%s) route config failed, [%s]", (char *)data, zerror(rc));
        goto ERR_RET;
    }

    /* 业务配置为空 */
    if (value_len == 0) {
        NLOG_ERROR("service (%s) route config is null", (char *)data);
        goto ERR_RET;
    }

    rdata = get_local_rdata((char *)data);
    if (rdata != NULL) {
        /* 如果和本地的路由数据时间戳一致，不更新 */
        if (rdata->route_meta->mtime == (uint64_t)stat->mtime) {
            free((void *)data);
            return;
        }
    }

    NLOG_INFO("receive service (%s) config", (char *)data);

    /* 处理配置 */
    ret = process_load_config((const char *)data, value, value_len, (uint64_t)stat->mtime);
    if (ret < 0) {
        NLOG_ERROR("process load config failed, ret [%d]", ret);
        goto ERR_RET;
    }

    /* 业务配置加载成功，需要处理路由请求任务 */
    process_route_task((char *)data);
    free((void *)data);
    return;

ERR_RET:
    delete_route_task((char *)data);  // 这里没有判断是否首次加载失败，会有重复调用的问题
    free((void *)data);
    return;
}


/**
 * @brief  获取业务配置信息
 * @info   只有新业务请求和NLB_EVENT_TYPE_LOAD_SERVICE事件会调用该函数
 * @return =0 成功 <0 失败
 */
int32_t load_service_config(const char *name)
{
    int32_t ret, result;
    char  path[NLB_PATH_MAX_LEN];
    char *ctx = NULL;

    if (!zk_connected()) {
        result = -1;
        NLOG_ERROR("zookeeper is not connected");
        goto ERR_EXIT;
    }

    NLOG_DEBUG("load service [%s] config", name);

    ctx = strdup(name);
    if (NULL == ctx) {
        NLOG_ERROR("No memory");
        result = -1;
        goto ERR_EXIT;
    }

    /* 调用zookeeper接口获取数据 */
    make_zk_service_path(name, path, NLB_PATH_MAX_LEN);
    ret = zoo_aget(get_zk_instance(), path, 0, nameservice_aget_completion, ctx);
    if (ret != ZOK) {
        NLOG_ERROR("get service (%s) from zookeeper failed, [%s]", name, zerror(ret));
        result = -2;
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:

    /* 获取业务配置失败，需要删除路由任务 */
    delete_route_task(name);
    if (ctx)
        free(ctx);

    return result;
}





