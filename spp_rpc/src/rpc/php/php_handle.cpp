
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


#include <php.h>
#include <php_ini.h>
#include <zend.h>
#include <php_config.h>
#include <sapi/embed/php_embed.h>

#if ZTS
#include <TSRM/TSRM.h>
#endif

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "spp_version.h"
#include "tlog.h"
#include "tstat.h"
#include "tcommu.h"
#include "serverbase.h"
#include "monitor.h"
#include "mt_incl.h"
#include "SyncMsg.h"
#include "SyncFrame.h"
#include "srpc.pb.h"
#include "srpc_log.h"
#include "srpc_comm.h"
#include "srpc_proto.h"
#include "srpc_channel.h"
#include "srpc_service.h"
#include "srpc_intf.h"
#include "php_handle.h"

#include "php_handle.h"

#define PHP_INI_FILE    "./php/php.ini"
#define PHP_ENTRY_FILE  "./php/entry.php"
#define LOG_SCREEN(lvl, fmt, args...)   do{ printf("[%d] ", getpid());printf(fmt, ##args); fflush(stdout);} while(0)

using namespace srpc;

#ifdef ZTS
void ***tsrm_ls;
#endif

static uint64_t g_loadfile_time;

// PHP全局析构函数
void srpc_php_handle_fini(void)
{
    zval z_retval, z_funcname;

    // 调用PHP全局析构函数
    ZVAL_STRING(&z_funcname, "service\fini", 1);

    call_user_function(EG(function_table), NULL, &z_funcname, &z_retval, 0, NULL TSRMLS_CC);
    zval_dtor(&z_retval);
    zval_dtor(&z_funcname);
}

// PHP全局初始化函数
int srpc_php_handler_init(const char *config)
{

    zval z_retval, z_funcname;
    zval *zp_config;
    zval *z_params[1];

    // 1. 初始化函数名变量
    ZVAL_STRING(&z_funcname, "service\\init", 1);

    // 2. 初始化参数变量
    MAKE_STD_ZVAL(zp_config);
    ZVAL_STRING(zp_config, (char *)config, 1);
    z_params[0]=zp_config;


    // 3. 调用用户空间初始化函数
    int ret =  call_user_function(EG(function_table), NULL,
                                     &z_funcname, &z_retval,
                                     1, z_params TSRMLS_CC);

    if (ret != SUCCESS)
    {
        LOG_SCREEN(LOG_ERROR, "Call php service::init failed.\n");
        return -1;
    }

    convert_to_long(&z_retval);
    ret = Z_LVAL_P(&z_retval);
    if (ret)
    {
        LOG_SCREEN(LOG_ERROR, "Call php service::init failed, ret [%d].\n", ret);
        return -2;
    }

    zval_ptr_dtor(&zp_config);
    zval_dtor(&z_retval);
    zval_dtor(&z_funcname);

    return ret;
}

void srpc_php_handler_loop(void)
{
    zval z_retval, z_funcname;

    ZVAL_STRING(&z_funcname, "service\\loop", 1);
    call_user_function(EG(function_table), NULL,
                            &z_funcname, &z_retval,
                            0, NULL TSRMLS_CC);

    zval_dtor(&z_funcname);
    zval_dtor(&z_retval);
}


// 处理函数
int srpc_php_handler_process(const string &method,  void* extInfo, int type,
                             const string &request, string &response)
{
    int ret = 0;
    zval z_retval, z_funcname;
    zval *zp_method, *zp_request, *zp_ext_info, *zp_is_json;
    zend_uint z_param_count = 4;
    zval *z_params[3];

    // 1. 初始化请求方法变量
    MAKE_STD_ZVAL(zp_method);
    ZVAL_STRINGL(zp_method, method.data(), method.size() , 1);

    // 2. 初始化请求报文变量
    MAKE_STD_ZVAL(zp_request);
    ZVAL_STRINGL(zp_request, request.data(), request.size() , 1);

    // 3. 初始化扩展信息变量
    MAKE_STD_ZVAL(zp_ext_info);
    array_init(zp_ext_info);

    // 4. 初始化协议类型
    MAKE_STD_ZVAL(zp_is_json);
    ZVAL_BOOL(zp_is_json, (type == 2));
    
    TConnExtInfo* info = (TConnExtInfo*) extInfo;
    add_assoc_long(zp_ext_info, "REMOTE_ADDR", info->remoteip_);
    add_assoc_long(zp_ext_info, "REMOTE_PORT", info->remoteport_);
    add_assoc_long(zp_ext_info, "LOCAL_ADDR", info->localip_);
    add_assoc_long(zp_ext_info, "LOCAL_PORT", info->localport_);
    add_assoc_long(zp_ext_info, "FD_TYPE", info->type_);
    add_assoc_long(zp_ext_info, "FD", info->fd_);
    add_assoc_long(zp_ext_info, "RECV_TIME_SEC", (long) info->recvtime_);
    add_assoc_long(zp_ext_info, "RECV_TIME_USEC",(long) info->tv_usec);

    // TODO: PHP统一入口函数
    ZVAL_STRING(&z_funcname, "service\\process", 1);

    // 4. 调用PHP用户态接口函数
    z_params[0] = zp_method; 
    z_params[1] = zp_request;
    z_params[2] = zp_is_json;
    z_params[3] = zp_ext_info;

    ret = call_user_function(EG(function_table), NULL,
                                &z_funcname, &z_retval,
                                z_param_count, z_params TSRMLS_CC);

    if (Z_TYPE_P(&z_retval) == IS_LONG) {
        NGLOG_ERROR("Php call failed: %s", errmsg((int)(Z_LVAL_P(&z_retval))));
        ret = FAILURE;
        goto EXIT;
    }

    if (Z_TYPE_P(&z_retval) != IS_STRING) {
        NGLOG_ERROR("Invalid php response, [%s].", method.c_str());
        ret = FAILURE;
        goto EXIT;
    }

    response.assign(Z_STRVAL_P(&z_retval), Z_STRLEN_P(&z_retval));

EXIT:
    zval_ptr_dtor(&zp_method);
    zval_ptr_dtor(&zp_request);
    zval_ptr_dtor(&zp_ext_info);
    zval_ptr_dtor(&zp_is_json);
    zval_dtor(&z_retval);
    zval_dtor(&z_funcname);

    return ret == SUCCESS ? SRPC_SUCCESS : SRPC_ERR_PHP_FAILED;
}

// 初始化PHP引擎, 开发者可以自己上传php.ini
int srpc_php_init(void)
{
    if (!access(PHP_INI_FILE, R_OK))
    {
        php_embed_module.php_ini_path_override = strdup(PHP_INI_FILE);
        LOG_SCREEN(LOG_INFO, "Set php ini path [%s].\n", PHP_INI_FILE);
    }

    php_embed_init(0, NULL PTSRMLS_CC);

    return 0;
}

// 加载php入口文件
int srpc_php_load_file(void)
{
    const char *filename = PHP_ENTRY_FILE;

    // 1. 检查文件是否有可读权限
    if (access(filename, R_OK))
    {
        LOG_SCREEN(LOG_ERROR, "Access file [%s] failed, [%m]", filename);
        return -1;
    }

    // 2. 获取文件的更新时间
    struct stat buf;
    int ret = stat(filename, &buf);
    if (ret)
    {
        LOG_SCREEN(LOG_ERROR, "Read php entry file [%s] stat failed, [%m]", filename);
        return -2;
    }

    g_loadfile_time = (uint64_t)buf.st_mtime;

    // 3. 加载php入口文件
    zend_first_try {
        // load necessary php files
        // suggest use require,not include
        char *include_script = NULL;
        spprintf(&include_script, 0, "require '%s';", filename);
        zend_eval_string(include_script, NULL, include_script TSRMLS_CC);
        efree(include_script);
        ret = 0;
    } zend_catch {
        LOG_SCREEN(LOG_ERROR, "Load [%s] failed\n", filename);
        php_embed_shutdown(TSRMLS_C);
        ret = -3;
    } zend_end_try ();

    return ret;
}


// 加载php5动态库
int srpc_php_dl_php_lib(void)
{

    void *handler   = NULL;
    char *path      = "./lib/libphp5.so";

    // 将libphp5.so的符号暴露给框架
    handler = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
    if (NULL == handler)
    {
        LOG_SCREEN(LOG_ERROR, "dlopen %s failed, (%s)\n", path, dlerror());
        return -1;
    }

    dlclose(handler);

    return 0;
}

// 终止PHP引擎
void srpc_php_end(void)
{
    php_embed_shutdown(TSRMLS_C);
}

bool srpc_check_memory_health(float water)
{
    long limit = PG(memory_limit);
    long used  = zend_memory_usage(1 TSRMLS_CC); 

    if (limit <= 0) {
        return true;
    }

    if ((used * 1.0)/limit > water) {
        return false;
    }

    return true;
}

