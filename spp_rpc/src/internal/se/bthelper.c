
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



#if  defined(__amd64__) || defined(__x86_64__)
#include "libunwind-x86_64.h"
#elif defined(__i386__)
#include "libunwind-x86.h"
#else
#error "Linux cpu arch not supported"
#endif

#include <string.h>
#include <stdio.h>
#include "memlog.h"
#include "backtrace.h"
#include "backtrace-supported.h"

typedef struct _tag_backtrace_state_node
{
    struct backtrace_state *state;
    char *path;
    void *next;
}bts_node_t;

typedef struct _tag_bts_list
{
    bts_node_t *head;
}bts_list_t;

typedef struct _tag_backtrace_args
{
    uintptr_t   pc;
    char *      func;
    char *      wbuf;
    unw_word_t  off;
    int         wbuf_len;
    int         wlen;
}backtrace_args_t;

struct backtrace_state *get_bts(bts_list_t *list, const char *path, s_heap_t *heap)
{
    int len;
    bts_node_t *bts;
    struct backtrace_state *state;

    if (NULL == list || NULL == path || path[0] == '\0') {
        return NULL;
    }

    // 查找是否已经有分配好的backtrace_state
    for (bts = list->head; bts != NULL; bts = bts->next) {
        if (!strcmp(path, bts->path)) {
            return bts->state;
        }
    }

    // 重新创建一个backtrace_state
    state = backtrace_create_state(path, 0, NULL, NULL);
    if (NULL == state) {
        return NULL;
    }

    // 加入链表
    bts = (bts_node_t *)s_heap_alloc(heap, sizeof(bts_node_t));
    if (NULL == bts) {
        return NULL;
    }

    len = strlen(path) + 1;
    bts->path   = (char *)s_heap_alloc(heap, sizeof(bts_node_t));
    if (NULL == bts->path) {
        return NULL;
    }

    strncpy(bts->path, path, len);
    bts->state  = state;
    bts->next   = list->head;
    list->head  = bts;

    return state;
}

/**
 * @brief backtrace错误回调函数
 * @info  执行错误，直接打印libunwind解析的信息
 */
void backtrace_err_cb(void *data, const char *msg, int errnum)
{
    int len;
    backtrace_args_t *args = (backtrace_args_t *)data;

    if (errnum == -1) {
        len = snprintf(args->wbuf, args->wbuf_len, "  0x%016lx (%s+%lu) -- <no debug info>\n",
                       (unsigned long)args->pc, args->func ? args->func : "?", (unsigned long)args->off);
        args->wlen = len;
    } else {
        len = snprintf(args->wbuf, args->wbuf_len, "  0x%016lx (%s+%lu) -- <%s>\n",
                       (unsigned long)args->pc, args->func ? args->func : "?", (unsigned long)args->off, msg);
        args->wlen = len;
    }
}

/**
 * @brief backtrace获取调试信息回调函数
 * @info  执行错误，直接打印libunwind解析的信息
 */
int backtrace_cb(void *data, uintptr_t pc __attribute__((unused)), const char *filename, int lineno, const char *function)
{
    int len;
    const char *func;
    backtrace_args_t *args = (backtrace_args_t *)data;

    if (function) {
        func = function;
    } else if (args->func) {
        func = args->func;
    } else {
        func = "?";
    }

    if (filename) {
        len = snprintf(args->wbuf, args->wbuf_len,
                       "  0x%016lx (%s+%lu) -- %s:%d\n",
                       (unsigned long)args->pc,
                       func,
                       (unsigned long)args->off,
                       filename,
                       lineno);
        args->wlen = len;
        return len;
    } else {
        len = snprintf(args->wbuf, args->wbuf_len, "  0x%016lx (%s+%lu) -- <no debug info>\n",
                       (unsigned long)args->pc, args->func, (unsigned long)args->off);
        args->wlen = len;
    }

    return len;
}


/**
 * @brief 输出调用栈信息到对应描述符
 * @info  使用libunwind\libbacktrace库
 */
int show_backtrace_fd(int fd, ucontext_t *ucontext, s_heap_t *heap, vma_list_t *vma_list)
{
    int  ret;
    unw_word_t   ip;
    unw_word_t   off;
    unw_cursor_t cursor;
    bts_list_t   bts_list = {NULL};
    const char  *path;

    ret = unw_init_local(&cursor, ucontext);
    if (ret < 0) {
        return -1;
    }
    
    do{
        char func[128];
        char wbuf[128];
        struct backtrace_state *state;
        backtrace_args_t args;

        memset(&args, 0, sizeof(backtrace_args_t));

        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        ret = unw_get_proc_name(&cursor, func, sizeof(func), &off);
        if (ret < 0) {
          return -2;
        }

        path = vma_get_bin_path(vma_list, (void *)ip);
        if (NULL == path) {
            ret = snprintf(wbuf, sizeof(wbuf), "  0x%016lx (%s+%lu) -- <no bin path>\n", (unsigned long)ip, func, (unsigned long)off);
            goto write_to_file;
        }
        
        state = get_bts(&bts_list, path, heap);
        if (NULL == state) {
            ret = snprintf(wbuf, sizeof(wbuf), "  0x%016lx (%s+%lu) -- <get backtrace_state err>\n", (unsigned long)ip, func, (unsigned long)off);
            goto write_to_file;
        }

        args.func       = func;
        args.off        = off;
        args.pc         = (uintptr_t)ip;
        args.wbuf       = wbuf;
        args.wbuf_len   = sizeof(wbuf);
        args.wlen       = 0;

        backtrace_pcinfo(state, (uintptr_t)ip, backtrace_cb, backtrace_err_cb, &args);

        ret = args.wlen;

write_to_file:
	if (ret)
            write_all(fd, wbuf, ret);
    } while ((ret = unw_step(&cursor)) > 0);

    return 0;
}


