
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_nlb_php.h"
#include "nlbapi.h"

/* If you declare any globals in php_nlb_php.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(nlb_php)
*/

/* True global resources - no need for thread safety here */
static int le_nlb_php;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("nlb_php.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_nlb_php_globals, nlb_php_globals)
    STD_PHP_INI_ENTRY("nlb_php.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_nlb_php_globals, nlb_php_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_nlb_php_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_nlb_php_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "nlb_php", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

PHP_FUNCTION(getroutebyname)
{
        char *service_name;
        char *type;
        int slen, ret;
        struct routeid route;
        char buf[64];

        if (ZEND_NUM_ARGS() != 1) {
                RETURN_NULL();
        }

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &service_name, &slen) == FAILURE) {
                RETURN_NULL();
        }

        ret = getroutebyname(service_name, &route);
        if (ret) {
                RETURN_NULL();
        }

        array_init(return_value);

        add_assoc_string(return_value, "ip", inet_ntoa(*(struct in_addr *)&route.ip), 1);
        snprintf(buf, sizeof(buf), "%u", route.port);
        add_assoc_string(return_value, "port", buf, 1);

        switch(route.type) {
                case NLB_PORT_TYPE_UDP:
                        type = "udp";
                        break;
                case NLB_PORT_TYPE_TCP:
                        type = "tcp";
                        break;
                case NLB_PORT_TYPE_ALL:
                        type = "all";
                        break;
                default:
                        type = "unkown";
                        break;

        }
        add_assoc_string(return_value, "type", type, 1);

        return;
}

PHP_FUNCTION(updateroute)
{
        zend_bool failed;
        char *ip;
        char *service_name;
        long cost;
        int slen, iplen;
        unsigned int nip;

        if (ZEND_NUM_ARGS() != 4) {
                return;
        }

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssbl", \
                                &service_name, &slen, &ip, &iplen, &failed, &cost) == FAILURE) {
                return;
        }

        nip = (unsigned int)inet_addr(ip);
        updateroute(service_name, nip, failed, (int)cost);
}


/* {{{ php_nlb_php_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_nlb_php_init_globals(zend_nlb_php_globals *nlb_php_globals)
{
	nlb_php_globals->global_value = 0;
	nlb_php_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(nlb_php)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(nlb_php)
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
PHP_RINIT_FUNCTION(nlb_php)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(nlb_php)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(nlb_php)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "nlb_php support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ nlb_php_functions[]
 *
 * Every user visible function must have an entry in nlb_php_functions[].
 */
const zend_function_entry nlb_php_functions[] = {
	PHP_FE(confirm_nlb_php_compiled,	NULL)		/* For testing, remove later. */
    PHP_FE(getroutebyname,              NULL)
    PHP_FE(updateroute,                 NULL)
	PHP_FE_END	/* Must be the last line in nlb_php_functions[] */
};
/* }}} */

/* {{{ nlb_php_module_entry
 */
zend_module_entry nlb_php_module_entry = {
	STANDARD_MODULE_HEADER,
	"nlb_php",
	nlb_php_functions,
	PHP_MINIT(nlb_php),
	PHP_MSHUTDOWN(nlb_php),
	PHP_RINIT(nlb_php),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(nlb_php),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(nlb_php),
	PHP_NLB_PHP_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_NLB_PHP
ZEND_GET_MODULE(nlb_php)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
