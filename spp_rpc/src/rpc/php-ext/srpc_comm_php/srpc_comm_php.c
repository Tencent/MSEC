
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
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>

#include "nlbapi.h"
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_srpc_comm_php.h"
#include "srpc_proto_php.h"

/* If you declare any globals in php_srpc_comm_php.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(srpc_comm_php)
*/

/* True global resources - no need for thread safety here */
static int le_srpc_comm_php;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("srpc_comm_php.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_srpc_comm_php_globals, srpc_comm_php_globals)
    STD_PHP_INI_ENTRY("srpc_comm_php.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_srpc_comm_php_globals, srpc_comm_php_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_srpc_comm_php_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_srpc_comm_php_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "srpc_comm_php", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/

/**
 * $buf = srpc_pack("Login.login", "hello world", 100);
 * if ($buf == NULL) {
 *     error
 * }
 */
PHP_FUNCTION(srpc_serialize)
{
    char *method_name;
    char *body_str;
    char *buf;
    int method_len, body_len, buf_len;
    long seq;

    if (ZEND_NUM_ARGS() != 3) {
        RETURN_NULL();
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &method_name, &method_len, &body_str, &body_len, &seq) == FAILURE) {
        RETURN_NULL();
    }

    buf_len = body_len + 4096;
    buf = (char *)emalloc(buf_len);
    if (NULL == buf) {
        RETURN_NULL();
    }

    if (srpc_php_pack(method_name, (uint64_t)seq, body_str, body_len, buf, &buf_len)) {
        efree(buf);
        RETURN_NULL();
    }

    RETVAL_STRINGL(buf, buf_len, 1);

    efree(buf);
}

/*
PHP_FUNCTION(srpc_unpack)
{
    char *buf;
    char *body_str;
    int buf_len, body_len, ret;
    uint64_t seq;
    int32_t  err;

    if (ZEND_NUM_ARGS() != 1) {
        RETURN_NULL();
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &buf, &buf_len) == FAILURE) {
        RETURN_NULL();
    }

    body_len = buf_len;
    body_str = emalloc(buf_len);
    if (NULL == body_str) {
        RETURN_NULL();
    }

    ret = srpc_php_unpack(buf, buf_len, body_str, &body_len, &seq, &err);
    if (ret) {
        efree(body_str);
        RETURN_NULL();
    }

    if (body_len)
        RETVAL_STRINGL(body_str, body_len, 1);
    else
        ZVAL_EMPTY_STRING(return_value);
    efree(body_str);
}
*/

#if 1
PHP_FUNCTION(srpc_deserialize)
{
    char *buf;
    char *body_str;
    int buf_len, body_len, ret;
    uint64_t seq;
    int32_t  err;

    array_init(return_value);

    if (ZEND_NUM_ARGS() != 1) {
        add_assoc_string(return_value, "errmsg", "Invalid parameter", 1);
        return;
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &buf, &buf_len) == FAILURE) {
        add_assoc_string(return_value, "errmsg", "Invalid parameter", 1);
        return;
    }

    body_len = buf_len;
    body_str = emalloc(buf_len);
    if (NULL == body_str) {
        add_assoc_string(return_value, "errmsg", "No memory", 1);
        return;
    }

    ret = srpc_php_unpack(buf, buf_len, body_str, &body_len, &seq, &err);
    if (ret) {
        efree(body_str);
        add_assoc_string(return_value, "errmsg", srpc_php_err_msg(ret), 1);
        return;
    }

    add_assoc_stringl(return_value, "body", body_str, body_len, 1);
    add_assoc_long(return_value, "seq", (long)seq);
    add_assoc_string(return_value, "errmsg", srpc_php_err_msg(err), 1);
    efree(body_str);
}
#endif

PHP_FUNCTION(srpc_check_pkg)
{
    int ret;    
    char *buf;
    int buf_len;

    if (ZEND_NUM_ARGS() != 1) {
        RETURN_NULL();
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &buf, &buf_len) == FAILURE) {
        RETURN_NULL();
    }

    ret = srpc_php_check_pkg_len(buf, buf_len);
    if (ret < 0) {
        RETURN_NULL();
    }

    RETURN_LONG((long)ret);
}

typedef struct _tag_nethandler
{
    struct sockaddr_in dst;
    int type;
    int fd;
    char *sendbuf;
    int sendlen;
    int sendbuflen;
    char *recvbuf;
    int recvlen;
    int recvbuflen;
    int timeout;
}nethandler_t;

static inline uint64_t covert_tv_2_ms(struct timeval *tv)
{
    return (tv->tv_sec*1000 + tv->tv_usec/1000);
}

static inline void covert_ms_2_tv(uint64_t tms, struct timeval *tv)
{
    tv->tv_sec  = tms/1000;
    tv->tv_usec = tms%1000*1000;
}


static int parse_host(const char *hostname, struct sockaddr_in *addr, int *type)
{
    struct routeid route;
    char *pos;
    char *host = strdup(hostname);

    if (pos = strchr(host, ':')) {
        *pos++ = '\0';
        addr->sin_addr.s_addr   = inet_addr(host);
        addr->sin_port          = htons((uint16_t)atoi(pos));
        addr->sin_family        = AF_INET;

        if (strstr(pos, "udp")) {
            *type = 1;
        } else {
            *type = 2;
        }

        return 0;
    }

    if (getroutebyname(hostname, &route)) {
        return -1;
    }

    addr->sin_family        = AF_INET;
    addr->sin_port          = htons(route.port);
    addr->sin_addr.s_addr   = route.ip;

    return 0;
}

static int socket_create(nethandler_t *handler)
{
    int flag, fd;
    struct sockaddr_in dst;

    if (NULL == handler) {
        return -1;
    }

    if (handler->type == 1) {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else {
        fd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (fd == -1) {
        return -2;
    }

    flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        close(fd);
        return -3;
    }

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK | O_NDELAY) == -1) {
        close(fd);
        return -4;
    }

    handler->fd = fd;

    return 0;
}

static int socket_connect(nethandler_t *handler)
{
    struct timeval begin, end;
    struct pollfd pollfd;
    int cost;

    if (handler->type == 1) { // udp
        return 0;
    }

    gettimeofday(&begin, NULL);
    if (connect(handler->fd, (struct sockaddr *)&handler->dst, sizeof(struct sockaddr_in)) == -1) {
        if (errno == EISCONN) {
            return 0;
        }

        if ((errno != EAGAIN) && (errno != EINTR) && (errno != EINPROGRESS)) {
            return -1;
        }

        pollfd.fd       = handler->fd;
        pollfd.events   = POLLOUT;
        pollfd.revents  = 0;

        if (poll(&pollfd, 1, handler->timeout) != 1) {
            return -2;
        }

        if (pollfd.revents & POLLOUT) {
            int rc, error;
            socklen_t len = sizeof(error);
            rc = getsockopt(handler->fd, SOL_SOCKET, SO_ERROR, &error, &len);
            if (rc == -1 || error) {
                return -3;
            }
        }
    }
    gettimeofday(&end, NULL);

    cost = end.tv_sec*1000 + end.tv_usec/1000 - begin.tv_sec*1000 - begin.tv_usec/1000;
    if (handler->timeout <= cost) {
        return -4;
    }

    handler->timeout -= cost;

    return 0;
}

static int socket_send(nethandler_t *handler)
{
    struct pollfd pollfd;
    struct timeval begin, end;
    int ret, cost;

    while ((handler->timeout > 0) && (handler->sendlen < handler->sendbuflen)) { 
        gettimeofday(&begin, NULL);

        pollfd.fd       = handler->fd;
        pollfd.events   = POLLOUT;
        pollfd.revents  = 0;

        if (poll(&pollfd, 1, handler->timeout) != 1) {
            return -1;
        }
        
        if (pollfd.revents & POLLOUT) {
            if (handler->type == 1) {
                ret = sendto(handler->fd, handler->sendbuf + handler->sendlen, handler->sendbuflen - handler->sendlen, 0, \
                             (struct sockaddr *)&handler->dst, sizeof(struct sockaddr_in));
            } else {
                ret = send(handler->fd, handler->sendbuf + handler->sendlen, handler->sendbuflen - handler->sendlen, 0);
            }

            if (ret == -1) {
                if ((errno != EAGAIN) && (errno != EINTR)) {
                    return -2;
                }
            } else {
                handler->sendlen += ret;
            }
        }

        gettimeofday(&end, NULL);
        cost = end.tv_sec*1000 + end.tv_usec/1000 - begin.tv_sec*1000 - begin.tv_usec/1000;
        if (handler->timeout <= cost) {
            return -3;
        }

        handler->timeout -= cost;
    }

    return 0;
}

static int socket_recv(nethandler_t *handler)
{
    struct pollfd pollfd;
    struct timeval begin, end;
    int ret, cost;

    do { 
        gettimeofday(&begin, NULL);
        pollfd.fd       = handler->fd;
        pollfd.events   = POLLIN;
        pollfd.revents  = 0;
        
        if (poll(&pollfd, 1, handler->timeout) != 1) {
            return -1;
        }
        
        if (pollfd.revents & POLLIN) {
            ret = recv(handler->fd, handler->recvbuf + handler->recvlen, handler->recvbuflen - handler->recvlen, 0);

            if (ret == -1) {
                if ((errno != EAGAIN) && (errno != EINTR)) {
                    return -2;
                }
            }

            handler->recvlen += (ret == -1 ? 0 : ret);

            ret = srpc_php_check_pkg_len(handler->recvbuf, handler->recvlen);
            if (ret < 0) {
                return -4;
            } else if (ret > 0) {
                return 0;
            }

            if (handler->recvlen == handler->recvbuflen) {
                char *buf;
                handler->recvbuflen = handler->recvbuflen * 2;
                buf = (char *)erealloc(handler->recvbuf, handler->recvbuflen);
                if (NULL == buf) {
                    efree(handler->recvbuf);
                    handler->recvbuf = NULL;
                    return -5;
                }

                handler->recvbuf = buf;
            }            
        }

        gettimeofday(&end, NULL);
        cost = end.tv_sec*1000 + end.tv_usec/1000 - begin.tv_sec*1000 - begin.tv_usec/1000;
            
        if (handler->timeout <= cost) {
            return -3;
        }

        handler->timeout -= cost;
    }while (handler->timeout > 0);

    return 0;
}


static int nethandler_send(nethandler_t *handler)
{
    return socket_send(handler);
}

static int nethandler_recv(nethandler_t *handler)
{
    return socket_recv(handler);
}

static void nethandler_destory(nethandler_t *handler)
{
    if (handler->fd != -1)
        close(handler->fd);

    if (NULL != handler->recvbuf)
        efree(handler->recvbuf);
}

static int nethandler_init(nethandler_t *handler, const char *hostname, const char *sendbuf, int sbuf_len, int timeout)
{
    handler->fd         = -1;
    handler->recvbuf    = NULL;
    handler->recvbuflen = 0;
    handler->recvlen    = 0;
    handler->sendbuf    = (char *)sendbuf;
    handler->sendbuflen = sbuf_len;
    handler->sendlen    = 0;
    handler->type       = 0;
    handler->timeout    = timeout > 0 ? timeout : 5000;

    if (parse_host(hostname, &handler->dst, &handler->type)) {
        return -1;
    }

    if (socket_create(handler)) {
        return -2;
    }

    if (socket_connect(handler)) {
        close(handler->fd);
        handler->fd = -1;
        return -3;
    }

    handler->recvbuf = (char *)emalloc(64*1024);
    if (NULL == handler->recvbuf) {
        close(handler->fd);
        handler->fd = -1;
        return -4;
    }

    handler->recvbuflen = 64*1024;
    return 0;
}

/**
 * @brief $rsp = srpc_sendrcv("127.0.0.1:100@tcp", req_body, timeout);
 * @      $rsp = srpc_sendrcv("login.ptlogin", req_body, timeout);
 * @    $rsp["errmsg"] == "success";
 * @    $rsp["body"] == "";
 */
PHP_FUNCTION(srpc_sendrcv)
{
    char *pkg;
    char *hostname;
    int pkg_len, host_len;
    int ret;
    long timeout;
    nethandler_t handler;

    if (ZEND_NUM_ARGS() != 3) {
        //add_assoc_string(return_value, "errmsg", "Invalid parameter", 1);    
        RETURN_NULL();
    }

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &hostname, &host_len, &pkg, &pkg_len, &timeout) == FAILURE) {
        //add_assoc_string(return_value, "errmsg", "Invalid parameter", 1);    
        RETURN_NULL();
    }

    ret = nethandler_init(&handler, hostname, pkg, pkg_len, (int)timeout);
    if (ret < 0) {
        //add_assoc_string(return_value, "errmsg", "Nethandler init falied", 1);    
        RETURN_NULL();
    }

    ret = nethandler_send(&handler);
    if (ret < 0) {
        //add_assoc_string(return_value, "errmsg", "Nethandler send falied", 1);    
        nethandler_destory(&handler);
        RETURN_NULL();
    }

    ret = nethandler_recv(&handler);
    if (ret < 0) {
        //add_assoc_string(return_value, "errmsg", "Nethandler recv falied", 1);    
        nethandler_destory(&handler);
        RETURN_NULL();
    }
    
    //add_assoc_stringl(return_value, "rsp", handler.recvbuf, handler.recvlen, 1);
    //add_assoc_string(return_value, "errmsg", "Success", 1);    
    RETVAL_STRINGL(handler.recvbuf, handler.recvlen, 1);
    nethandler_destory(&handler);

    return ;
}


/* {{{ php_srpc_comm_php_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_srpc_comm_php_init_globals(zend_srpc_comm_php_globals *srpc_comm_php_globals)
{
	srpc_comm_php_globals->global_value = 0;
	srpc_comm_php_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(srpc_comm_php)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(srpc_comm_php)
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
PHP_RINIT_FUNCTION(srpc_comm_php)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(srpc_comm_php)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(srpc_comm_php)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "srpc_comm_php support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ srpc_comm_php_functions[]
 *
 * Every user visible function must have an entry in srpc_comm_php_functions[].
 */
const zend_function_entry srpc_comm_php_functions[] = {
	PHP_FE(confirm_srpc_comm_php_compiled,	NULL)		/* For testing, remove later. */
	PHP_FE(srpc_serialize,	    NULL)
    PHP_FE(srpc_deserialize,    NULL)
    PHP_FE(srpc_check_pkg,      NULL)
	PHP_FE(srpc_sendrcv,	    NULL)
	PHP_FE_END	/* Must be the last line in srpc_comm_php_functions[] */
};
/* }}} */

/* {{{ srpc_comm_php_module_entry
 */
zend_module_entry srpc_comm_php_module_entry = {
	STANDARD_MODULE_HEADER,
	"srpc_comm_php",
	srpc_comm_php_functions,
	PHP_MINIT(srpc_comm_php),
	PHP_MSHUTDOWN(srpc_comm_php),
	PHP_RINIT(srpc_comm_php),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(srpc_comm_php),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(srpc_comm_php),
	PHP_SRPC_COMM_PHP_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SRPC_COMM_PHP
ZEND_GET_MODULE(srpc_comm_php)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
