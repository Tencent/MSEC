
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


#include "nlbapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Python.h>

#include "srpc_proto_py.h"

typedef struct _tag_nethandler
{
    struct sockaddr_in dst;
    int type;
    int fd;
    char *sendbuf;
    int sendlen;
    int sendbuflen;
    char *recvbuf;
    char *host;
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

    if ((pos = strchr(host, ':'))) {
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

            ret = srpc_py_check_pkg_len(handler->recvbuf, handler->recvlen);
            if (ret < 0) {
                return -4;
            } else if (ret > 0) {
                return 0;
            }

            if (handler->recvlen == handler->recvbuflen) {
                char *buf;
                handler->recvbuflen = handler->recvbuflen * 2;
                buf = (char *)realloc(handler->recvbuf, handler->recvbuflen);
                if (NULL == buf) {
                    free(handler->recvbuf);
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
        free(handler->recvbuf);

    free(handler->host);
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
    handler->host       = strdup(hostname);

    if (parse_host(hostname, &handler->dst, &handler->type)) {
        return -1;
    }

    if (socket_create(handler)) {
        return -2;
    }

    if (socket_connect(handler)) {
        updateroute(handler->host, (*(uint32_t *)&handler->dst.sin_addr), 1, 0);
        close(handler->fd);
        handler->fd = -1;
        free(handler->host);
        return -3;
    }

    handler->recvbuf = (char *)malloc(64*1024);
    if (NULL == handler->recvbuf) {
        close(handler->fd);
        handler->fd = -1;
        free(handler->host);
        return -4;
    }

    handler->recvbuflen = 64*1024;
    return 0;
}

PyObject* wrap_srpc_serialize(PyObject* self, PyObject* args)
{
    PyObject *buf;
    Py_buffer body_buf;
    char *method_name;
    int   buf_len;
    int   seq;

    if (PyTuple_GET_SIZE(args) != 3) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (!PyArg_ParseTuple(args, "ss*i", &method_name, &body_buf, &seq)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    buf_len = body_buf.len + 8196;
    buf = PyString_FromStringAndSize((char *)0, buf_len);
    if (NULL == buf) {
        PyBuffer_Release(&body_buf);
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (srpc_py_pack(method_name, (uint64_t)seq, body_buf.buf, body_buf.len, PyString_AS_STRING(buf), &buf_len)) {
        PyBuffer_Release(&body_buf);
        Py_DECREF(buf);
        Py_INCREF(Py_None);
        return Py_None;
    }

    PyBuffer_Release(&body_buf);
    _PyString_Resize(&buf, buf_len);
    return buf;
}

PyObject* wrap_srpc_deserialize(PyObject* self, PyObject* args)
{
    uint64_t seq;
    int ret, len;
    int32_t  err;
    Py_buffer pkg_buf;
    PyObject *body_buf;
    PyObject *ret_dict;

    if (PyTuple_GET_SIZE(args) != 1) {
        return Py_BuildValue("{s:s}", "errmsg", "Invalid parameter");
    }

    if (!PyArg_ParseTuple(args, "s*", &pkg_buf)) {
        return Py_BuildValue("{s:s}", "errmsg", "Invalid parameter");
    }

    body_buf = PyString_FromStringAndSize((char *)0, pkg_buf.len);
    if (NULL == body_buf) {
        PyBuffer_Release(&pkg_buf);
        return Py_BuildValue("{s:s}", "errmsg", "No memory");
    }

    len = pkg_buf.len;
    ret = srpc_py_unpack((char *)pkg_buf.buf, pkg_buf.len, PyString_AS_STRING(body_buf), &len, &seq, &err);
    if (ret) {
        PyBuffer_Release(&pkg_buf);
        Py_DECREF(body_buf);
        return Py_BuildValue("{s:s}", "errmsg", srpc_py_err_msg(ret));
    }

    _PyString_Resize(&body_buf, len);
    ret_dict = Py_BuildValue("{s:O,s:i,s:s}", "body", body_buf, "seq", (int)seq, "errmsg", srpc_py_err_msg(err));
    PyBuffer_Release(&pkg_buf);
    Py_XDECREF(body_buf);

    return ret_dict;
}

PyObject* wrap_srpc_check_pkg(PyObject* self, PyObject* args)
{
    int ret;
    Py_buffer pkg_buf;

    if (PyTuple_GET_SIZE(args) != 1) {
        return Py_BuildValue("i", -1);
    }

    if (!PyArg_ParseTuple(args, "s*", &pkg_buf)) {
        return Py_BuildValue("i", -1);
    }

    ret = srpc_py_check_pkg_len(pkg_buf.buf, pkg_buf.len);
    if (ret < 0) {
        PyBuffer_Release(&pkg_buf);
        return Py_BuildValue("i", -1);
    }

    PyBuffer_Release(&pkg_buf);
    return Py_BuildValue("i", ret);
}

PyObject* wrap_srpc_sendrcv(PyObject* self, PyObject* args)
{
    int ret, timeout;
    char *hostname;
    Py_buffer send_buf;
    nethandler_t handler;
    PyObject *obj;

    if (PyTuple_GET_SIZE(args) != 3) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if (!PyArg_ParseTuple(args, "ss*i", &hostname, &send_buf, &timeout)) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    ret = nethandler_init(&handler, hostname, (char *)send_buf.buf, send_buf.len, (int)timeout);
    if (ret < 0) {
        PyBuffer_Release(&send_buf);
        Py_INCREF(Py_None);
        return Py_None;
    }

    ret = nethandler_send(&handler);
    if (ret < 0) {
        updateroute(handler.host, (*(uint32_t *)&handler.dst.sin_addr), 1, 0);
        nethandler_destory(&handler);
        PyBuffer_Release(&send_buf);
        Py_INCREF(Py_None);
        return Py_None;
    }

    ret = nethandler_recv(&handler);
    if (ret < 0) {
        updateroute(handler.host, (*(uint32_t *)&handler.dst.sin_addr), 1, 0);
        nethandler_destory(&handler);
        PyBuffer_Release(&send_buf);
        Py_INCREF(Py_None);
        return Py_None;
    }
    
    updateroute(handler.host, (*(uint32_t *)&handler.dst.sin_addr), 1, 0);
    obj = Py_BuildValue("s#", handler.recvbuf, handler.recvlen);;
    PyBuffer_Release(&send_buf);
    nethandler_destory(&handler);

    return obj;
}

static PyMethodDef srpc_comm_Methods[] =
{
  {"srpc_serialize",    wrap_srpc_serialize,    METH_VARARGS, "srpc_serialize"},
  {"srpc_deserialize",  wrap_srpc_deserialize,  METH_VARARGS, "srpc_deserialize"},
  {"srpc_check_pkg",    wrap_srpc_check_pkg,    METH_VARARGS, "srpc_check_pkg"},
  {"srpc_sendrcv",      wrap_srpc_sendrcv,      METH_VARARGS, "srpc_sendrcv"},
  {NULL, NULL}
};

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef srpc_comm_Module = {
    PyModuleDef_HEAD_INIT,
    "srpc_comm_py",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,   /* size of per-interpreter state of the module,
             or -1 if the module keeps state in global variables. */
    srpc_comm_Methods
};

// The initialization function must be named PyInit_name()
extern "C" PyMODINIT_FUNC PyInit_srpc_comm_py(void)
{
    return PyModule_Create(&srpc_comm_Module);
}

#else

extern "C" PyMODINIT_FUNC initsrpc_comm_py()
{
  PyObject* m;
  m = Py_InitModule("srpc_comm_py", srpc_comm_Methods);
}

#endif

