
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


/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_monitor_php.h"
#include "monitor_client.h"

/* If you declare any globals in php_monitor_php.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(monitor_php)
*/

/* True global resources - no need for thread safety here */
static int le_monitor_php;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("monitor_php.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_monitor_php_globals, monitor_php_globals)
    STD_PHP_INI_ENTRY("monitor_php.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_monitor_php_globals, monitor_php_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_monitor_php_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_monitor_php_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "monitor_php", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/
static char *g_php_service_name = NULL;

static int service_name(char *buff, int len)
{
    char *name_pos;
    char *pos;
    char path[256];

    if (NULL == buff || len < 3) {
        return -1;
    }

    getcwd(path, sizeof(path));
    if (strncmp(path, "/msec/", 6)) {
        return -2;
    }

    name_pos = path + 6;
    pos = index(name_pos, '/');
    if (NULL == pos) {
        return -3;
    }

    *pos = '.';

    pos = index(pos, '/');
    if (NULL == pos) {
        return -4;
    }

    *pos++ = '\0';

    if (strncmp(pos, "bin", 4)) {
        return -5;
    }

    if (strlen(name_pos) >= len) {
        return -6;
    }

    strncpy(buff, name_pos, len);

    return 0;
}

static int msec_service_name(char *buff, int len)
{
    const char *default_service_name = "default.default";
    if (NULL == buff || len < (strlen(default_service_name) + 1)) {
        return -1;
    }

    if (service_name(buff, len) < 0) {
        strncpy(buff, default_service_name, len);
    }

    return 0;
}


PHP_FUNCTION(attr_report)
{
    char *attr_name;
    char aname[128];
    int alen;
    long val = 1;

    if (ZEND_NUM_ARGS() == 2) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &attr_name, &alen, &val) == FAILURE) {
            return;
        }
    }else if (ZEND_NUM_ARGS() == 1) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &attr_name, &alen) == FAILURE) {
            return;
        }
    } else {
        return;
    }

    snprintf(aname, sizeof(aname), "usr.%s", attr_name);
    Monitor_Add(g_php_service_name, aname, val);

    return;
}

PHP_FUNCTION(rpc_report)
{
    char *attr_name;
    char aname[128];
    int alen;
    long val = 1;

    if (ZEND_NUM_ARGS() == 2) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &attr_name, &alen, &val) == FAILURE) {
            return;
        }
    }else if (ZEND_NUM_ARGS() == 1) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &attr_name, &alen) == FAILURE) {
            return;
        }
    } else {
        return;
    }

    snprintf(aname, sizeof(aname), "frm.rpc %s", attr_name);
    Monitor_Add(g_php_service_name, aname, val);

    return;
}



PHP_FUNCTION(attr_set)
{
    char *attr_name;
    char aname[128];
    int alen;
    long val;

    if (ZEND_NUM_ARGS() != 2) {
        return;
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &attr_name, &alen, &val) == FAILURE) {
        return;
    }

    snprintf(aname, sizeof(aname), "usr.%s", attr_name);
    Monitor_Set(g_php_service_name, aname, val);

    return;
}

PHP_FUNCTION(rpc_set)
{
    char *attr_name;
    char aname[128];
    int alen;
    long val;

    if (ZEND_NUM_ARGS() != 2) {
        return;
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &attr_name, &alen, &val) == FAILURE) {
        return;
    }

    snprintf(aname, sizeof(aname), "frm.rpc %s", attr_name);
    Monitor_Set(g_php_service_name, aname, val);

    return;
}




/* {{{ php_monitor_php_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_monitor_php_init_globals(zend_monitor_php_globals *monitor_php_globals)
{
	monitor_php_globals->global_value = 0;
	monitor_php_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(monitor_php)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(monitor_php)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(monitor_php)
{
    if (NULL == g_php_service_name) {
        g_php_service_name = (char *)pemalloc(256, 1);
        if (NULL == g_php_service_name) {
            return FAILURE;
        }

        if (msec_service_name(g_php_service_name, 256)) {
            strncpy(g_php_service_name, "default.default", sizeof("default.default"));
        }
    }

	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(monitor_php)
{
    if (g_php_service_name) {
        pefree(g_php_service_name, 1);
        g_php_service_name = NULL;
    }
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(monitor_php)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "monitor_php support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ monitor_php_functions[]
 *
 * Every user visible function must have an entry in monitor_php_functions[].
 */
const zend_function_entry monitor_php_functions[] = {
	PHP_FE(confirm_monitor_php_compiled,	NULL)		/* For testing, remove later. */
    PHP_FE(attr_report,                     NULL)
    PHP_FE(attr_set,                        NULL)
    PHP_FE(rpc_report,                      NULL)
    PHP_FE(rpc_set,                         NULL)
	PHP_FE_END	/* Must be the last line in monitor_php_functions[] */
};
/* }}} */

/* {{{ monitor_php_module_entry
 */
zend_module_entry monitor_php_module_entry = {
	STANDARD_MODULE_HEADER,
	"monitor_php",
	monitor_php_functions,
	PHP_MINIT(monitor_php),
	PHP_MSHUTDOWN(monitor_php),
	PHP_RINIT(monitor_php),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(monitor_php),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(monitor_php),
	PHP_MONITOR_PHP_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MONITOR_PHP
ZEND_GET_MODULE(monitor_php)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
