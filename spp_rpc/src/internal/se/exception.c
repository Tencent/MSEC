
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


#define _GNU_SOURCE

#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <ucontext.h>
#include <time.h>

#include "vma.h"
#include "mem.h"
#include "memlog.h"
#include "bthelper.h"
#include "exception.h"

#ifndef PATH_MAX
#define PATH_MAX 256
#endif

#define SIG_DEFAULT_LOG_PATH    "/var/signal_exception"
#define SIG_STACK_SIZE          (16*1024)                       /* 信号函数栈大小 */
#define SIG_STACK_AGLIN_SIZE    (1<<20)                         /* 栈基址对齐大小 */
#define SIG_STACK_PROTECT_SIZE  (SIG_STACK_AGLIN_SIZE*4)        /* 信号函数栈保护区域大小 */
#define SIG_HEAP_SIZE           (16*1024*1024)                  /* 用于信号处理函数使用的堆空间 */
#define SIG_RO_AREA_MAGIC       (0x296f7228)                    /* (ro) */
#define SIG_RO_CB_OFFSET        (4)                             /* 回调函数存放地址 */
#define SIG_RO_PREFIX_OFFSET    (256)                           /* 前缀存放地址 */

#define PTR_AGLIN(_ptr, _aglin) (void *)((unsigned long)(_ptr) / SIG_STACK_AGLIN_SIZE * SIG_STACK_AGLIN_SIZE)

typedef struct _tag_signal_cb_des_t
{
    signal_cb   cb;
    void *      args;
}signal_cb_des_t;

/**
 * @brief 获取线程PID
 */
pid_t syscall_gettid(void)
{
    return syscall(SYS_gettid);
}

/**
 * @brief 重置信号处理函数
 */
void reset_signal_handler(void)
{
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
}

/**
 * @brief 只读区域初始化
 */
void ro_area_init(void *addr, int size, const char *prefix, signal_cb_des_t *cb_desc)
{
    memset(addr, 0, size);
    *(int *)addr = SIG_RO_AREA_MAGIC;
    memcpy((char *)addr + SIG_RO_CB_OFFSET, cb_desc, sizeof(signal_cb_des_t));
    snprintf((char *)addr + SIG_RO_PREFIX_OFFSET, size - SIG_RO_PREFIX_OFFSET, "%s", prefix);
    mprotect(addr, size, PROT_READ);
}

/**
 * @brief  获取只读区域基址
 */
void *ro_area(void)
{
#if  defined(__amd64__) || defined(__x86_64__)
    register void *sp __asm__ ("rsp");
#elif defined(__i386__)
    register void *sp __asm__ ("esp");
#else
#error "Linux cpu arch not supported"
#endif

    void *ss_start;
    void *ro_start;

    ss_start = PTR_AGLIN(sp, SIG_STACK_AGLIN_SIZE);
    ro_start = (void *)((char *)ss_start + SIG_STACK_SIZE);

    return ro_start;
}

/**
 * @brief 获取信号日志文件前缀
 */
const char *get_log_prefix(void)
{
    void *ro_start;

    ro_start = ro_area();
    return (char *)ro_start + SIG_RO_PREFIX_OFFSET;
}

/**
 * @brief 获取信号回调函数
 */
signal_cb_des_t *get_register_cb(void)
{
    void *ro_start;
    signal_cb_des_t *cb_des;

    ro_start = ro_area();
    cb_des   = (signal_cb_des_t *)((char *)ro_start + SIG_RO_CB_OFFSET);

    if (cb_des->cb == NULL) {
        return NULL;
    }

    return cb_des;
}

/**
 * @brief 调用注册的信号回调函数
 */
void call_register_cb(int signo)
{
    signal_cb_des_t *cb_des;

    cb_des = get_register_cb();
    if (cb_des)
        cb_des->cb(signo, cb_des->args);
}



/**
 * @brief 创建LOG目录
 */
int create_log_dir(time_t now)
{
    int ret;
    char path[PATH_MAX];
    const char *prefix = get_log_prefix();

    snprintf(path, sizeof(path), "%s/%llu-%d-%d", prefix, (unsigned long long)now, getpid(), syscall_gettid());

    ret = mkdir(path, 0600);
    if (ret == -1) {
        return -1;
    }

    return 0;
}

/**
 * @brief 打开backtrace.dat文件，没有就创建
 */
int open_backtrace_file(time_t now)
{
    int fd;
    char path[PATH_MAX];
    const char *prefix = get_log_prefix();

    snprintf(path, sizeof(path), "%s/%llu-%d-%d/backtrace.dat", prefix, (unsigned long long)now, getpid(), syscall_gettid());

    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return -1;
    }

    return fd;
}

/**
 * @brief 打开memlog.dat文件，没有就创建
 */
int open_memlog_file(time_t now)
{
    int fd;
    char path[PATH_MAX];
    const char *prefix = get_log_prefix();

    snprintf(path, sizeof(path), "%s/%llu-%d-%d/memlog.dat", prefix, (unsigned long long)now, getpid(), syscall_gettid());

    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return -1;
    }

    return fd;
}

/**
 * @brief 打开maps.dat文件，没有就创建
 */
int open_maps_file(time_t now)
{
    int fd;
    char path[PATH_MAX];
    const char *prefix = get_log_prefix();

    snprintf(path, sizeof(path), "%s/%llu-%d-%d/maps.dat", prefix, (unsigned long long)now, getpid(), syscall_gettid());

    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return -1;
    }

    return fd;
}

/**
 * @brief 打开signal_info.dat文件，没有就创建
 */
int open_signal_info_file(time_t now)
{
    int fd;
    char path[PATH_MAX];
    const char *prefix = get_log_prefix();

    snprintf(path, sizeof(path), "%s/%llu-%d-%d/signal_info.dat", prefix, (unsigned long long)now, getpid(), syscall_gettid());

    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return -1;
    }

    return fd;
}

/**
 * @brief 检查指定目录是否存在
 */
int check_dir_exist(const char* path)
{
    if (!path) {
        return 0;
    }

    DIR* dir = opendir(path);
    if (!dir) {
        return 0;
    } else {
        closedir(dir);
        return 1;
    } 
}


/**
 * @brief 检查并创建目录
 */
int check_and_mkdir(const char *path)
{
    if (!path) {
        return 0;
    }

    if (!check_dir_exist(path)) {
        if (mkdir(path, 0755) < 0)  {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief 递归创建目录
 */
int32_t mkdir_recursive(const char *absolute_path)
{
    int32_t len;
    char    path[PATH_MAX];
    const char *pos;

    if ((NULL == absolute_path) || (absolute_path[0] != '/')) {
        return -1;
    }

    /* 循环创建目录 */
    pos = absolute_path + 1;
    while (NULL != (pos = index(pos, '/'))) {
        len = pos - absolute_path;

        if (len >= PATH_MAX) {
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


/**
 * @brief 保存memlog日志
 */
int save_memlog(time_t now, vma_list_t *vma_list)
{
    int ret;
    int fd;

    fd = open_memlog_file(now);
    if (fd < 0) {
        return -1;
    }

    ret = show_memlog_fd(fd, vma_list);

    close(fd);

    return ret;
}

/**
 * @brief 保存/proc/self/maps
 */
int save_maps(time_t now, vma_list_t *vma_list)
{
    int ret;
    int fd;

    fd = open_maps_file(now);
    if (fd < 0) {
        return -1;
    }

    ret = show_vma_list_fd(fd, vma_list);

    close(fd);

    return ret;
}

/**
 * @brief 读取启动命令行
 */
int read_comm(char *buf, int len)
{
    int fd;
    int rlen;
    int loop;

    fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    rlen = read_fd(fd, buf, len);
    if (rlen < 0) {
        return -2;
    }

    loop = 0;
    while(loop < rlen) {
        if (buf[loop] == '\0') {
            buf[loop] = ' ';
        }

        loop++;
    }

    buf[rlen - 1] = '\0';

    return rlen;
}

/**
 * @brief 打印异常信号详细信息
 */
int show_signal_info_fd(int fd, siginfo_t *signal_info, ucontext_t *ucontext, s_heap_t *heap)
{
    int   ret;
    int   blen, len = 0;
    char *buf;
    char  comm[256];

    if (NULL == signal_info || NULL == ucontext || NULL == heap) {
        return -1;
    }

    // 1. 分配空间用于组装字符串
    blen = 10240;
    buf  = s_heap_alloc(heap, blen);
    if (NULL == buf) {
        return -2;
    }

    // 2. 读取进程启动命令行
    ret = read_comm(comm, sizeof(comm));
    if (ret < 0) {
        return -3;
    }

    // 3. 打印信号上下文信息
    len += snprintf(buf+len, blen-len, "[ %lu ] Pid: %d, Tid: %d, comm: %s\n",
                    time(NULL), getpid(), syscall_gettid(), comm);

    if (signal_info->si_code <= 0) {
        len += snprintf(buf+len, blen-len, "Signal: %d errno: %d code: %d was generated by a process, Pid: %d, Uid: %d\n",
                        signal_info->si_signo,
                        signal_info->si_errno,
                        signal_info->si_code,
                        signal_info->si_pid,
                        signal_info->si_uid);
    } else {
        len += snprintf(buf+len, blen-len, "Signal: %d errno: %d code: %d addr_ref: 0x%lx\n",
                        signal_info->si_signo,
                        signal_info->si_errno,
                        signal_info->si_code,
                        (long)signal_info->si_addr);
    }

    len += snprintf(buf+len, blen-len, "Registers info:\n");

#if  defined(__amd64__) || defined(__x86_64__)
    len += snprintf(buf+len, blen-len,
                    "%-16s0x%-16llx\n" "%-16s0x%-16llx\n" "%-16s0x%-16llx\n"
                    "%-16s0x%-16llx\n" "%-16s0x%-16llx\n" "%-16s0x%-16llx\n"
                    "%-16s0x%-16llx\n" "%-16s0x%-16llx\n" "%-16s0x%-16llx\n"
                    "%-16s0x%-16llx\n" "%-16s0x%-16llx\n" "%-16s0x%-16llx\n"
                    "%-16s0x%-16llx\n" "%-16s0x%-16llx\n" "%-16s0x%-16llx\n"
                    "%-16s0x%-16llx\n" "%-16s0x%-16llx\n" "%-16s0x%-16llx\n",
                    "rax", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RAX],
                    "rbx", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RBX],
                    "rcx", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RCX],
                    "rdx", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RDX],
                    "rsi", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RSI],
                    "rdi", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RDI],
                    "rbp", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RBP],
                    "rsp", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RSP],
                    "r8",  (unsigned long long)ucontext->uc_mcontext.gregs[REG_R8],
                    "r9",  (unsigned long long)ucontext->uc_mcontext.gregs[REG_R9],
                    "r10", (unsigned long long)ucontext->uc_mcontext.gregs[REG_R10],
                    "r11", (unsigned long long)ucontext->uc_mcontext.gregs[REG_R11],
                    "r12", (unsigned long long)ucontext->uc_mcontext.gregs[REG_R12],
                    "r13", (unsigned long long)ucontext->uc_mcontext.gregs[REG_R13],
                    "r14", (unsigned long long)ucontext->uc_mcontext.gregs[REG_R14],
                    "r15", (unsigned long long)ucontext->uc_mcontext.gregs[REG_R15],
                    "rip", (unsigned long long)ucontext->uc_mcontext.gregs[REG_RIP],
                    "eflags", (unsigned long long)ucontext->uc_mcontext.gregs[REG_EFL]);
#elif defined(__i386__)
    len += snprintf(buf+len, blen-len,
                    "%-16s0x%-16x\n" "%-16s0x%-16x\n" "%-16s0x%-16x\n"
                    "%-16s0x%-16x\n" "%-16s0x%-16x\n" "%-16s0x%-16x\n"
                    "%-16s0x%-16x\n" "%-16s0x%-16x\n"
                    "%-16s0x%-16x\n" "%-16s0x%-16x\n",
                    "eax", ucontext->uc_mcontext.gregs[REG_EAX],
                    "ecx", ucontext->uc_mcontext.gregs[REG_ECX],
                    "edx", ucontext->uc_mcontext.gregs[REG_EDX],
                    "ebx", ucontext->uc_mcontext.gregs[REG_EBX],
                    "esp", ucontext->uc_mcontext.gregs[REG_ESP],
                    "ebp", ucontext->uc_mcontext.gregs[REG_EBP],
                    "esi", ucontext->uc_mcontext.gregs[REG_ESI],
                    "edi", ucontext->uc_mcontext.gregs[REG_EDI],
                    "eip", ucontext->uc_mcontext.gregs[REG_EIP],
                    "eflags", ucontext->uc_mcontext.gregs[REG_EFL]);
#else
#error "Linux cpu arch not supported"
#endif

    write_all(fd, buf, len);

    return 0;
}

/**
 * @brief 保存信号的上下文
 */
int save_signal_info(time_t now, siginfo_t *signal_info, ucontext_t *ucontext, s_heap_t *heap)
{
    int ret;
    int fd;

    fd = open_signal_info_file(now);
    if (fd < 0) {
        return -1;
    }

    ret = show_signal_info_fd(fd, signal_info, ucontext, heap);

    close(fd);

    return ret;
}

/**
 * @brief 保存调用栈日志
 */
int save_backtrace(time_t now, ucontext_t *ucontext, s_heap_t *heap, vma_list_t *vma_list)
{
    int ret;
    int fd;

    fd = open_backtrace_file(now);
    if (fd < 0) {
        return -1;
    }

    ret = show_backtrace_fd(fd, ucontext, heap, vma_list);

    close(fd);

    return ret;
}

/**
 * @brief 信号处理主函数
 * @info  
 */
void signal_handler(int signal_no, siginfo_t *signal_info, void *ucontext)
{
    int ret;
    time_t now;
    s_heap_t *heap;
    vma_list_t *vma_list;

    if (NULL == signal_info || NULL == ucontext || 0 == signal_no) {
        reset_signal_handler();
        return;
    }

    /**
     * 先执行安全的操作，不会导致再core
     */

    // 1. 初始化heap,后续使用
    heap = s_heap_init(SIG_HEAP_SIZE);
    if (NULL == heap) {
        reset_signal_handler();
        return;
    }

    // 2. 读取vma信息
    vma_list = vma_list_create(heap);
    if (NULL == vma_list) {
        reset_signal_handler();
        s_heap_destory(heap);
        return;
    }

    // 3. 创建目录
    now = time(NULL);
    ret = create_log_dir(now);
    if (ret < 0) {
        reset_signal_handler();
        s_heap_destory(heap);
        return;
    }

    // 4. 保存memlog
    save_memlog(now, vma_list);

    // 5. 保存vma信息
    save_maps(now, vma_list);

    // 6. 保存信号上下文信息
    save_signal_info(now, signal_info, (ucontext_t *)ucontext, heap);

    // 7. 先重置信号处理函数，后面的操作可能导致再次coredump
    reset_signal_handler();


    /**
     * @brief 下面的信息可能导致再次core
     */

    // 8. 保存调用栈信息
    save_backtrace(now, (ucontext_t *)ucontext, heap, vma_list);

    // 9. 释放堆空间
    s_heap_destory(heap);

    // 10. 调用注册的通知函数
    call_register_cb(signal_no);

}

/**
 * @brief 替换信号处理函数的栈
 * @info  虚拟地址格式
 * @      | protect | stack | rdonly | protect |
 */
int signal_stack_init(const char *prefix, signal_cb_des_t *cb_desc)
{
    int     ret;
    int     prot    = PROT_READ | PROT_WRITE;
    int     flags   = MAP_PRIVATE | MAP_ANON;
    int     memsize;
    int     rosize;
    void *  mem;
    char *  ss_start;
    char *  ss_end;
    char *  ro_start;
    char *  ro_end;
    char *  p1_start;
    char *  p1_end;
    char *  p2_start;
    char *  p2_end;
    stack_t stack;

    /* 初始化LOG前缀 */
    if (NULL == prefix || prefix[0] == '\0') {
        prefix = SIG_DEFAULT_LOG_PATH;
    }else if (prefix[0] != '/') {
        return -1;
    }

    /* 创建LOG主目录 */
    ret = mkdir_recursive(prefix);
    if (ret < 0) {
        return -2;
    }

    /* 申请栈虚拟内存 */
    rosize  = getpagesize();
    memsize = SIG_STACK_PROTECT_SIZE + SIG_STACK_SIZE + rosize;
    mem     = mmap(NULL, memsize, prot, flags, -1, 0);
    if (mem == (void *)MAP_FAILED) {
        return -1;
    }

    /* 计算各个区间的范围 */
    p1_start = (char *)mem;
    p1_end   = (char *)PTR_AGLIN(p1_start + SIG_STACK_PROTECT_SIZE/2, SIG_STACK_AGLIN_SIZE);
    ss_start = p1_end;
    ss_end   = ss_start + SIG_STACK_SIZE;
    ro_start = ss_end;
    ro_end   = ro_start + rosize;
    p2_start = ro_end;
    p2_end   = (char *)mem + memsize;
    ro_area_init(ro_start, rosize, prefix, cb_desc);

    /* 设置栈保护区域 */
    mprotect(p1_start, p1_end - p1_start, PROT_NONE);
    mprotect(p2_start, p2_end - p2_start, PROT_NONE);

    /* 替换信号处理函数的栈 */
    stack.ss_sp     = (void *)ss_start;
    stack.ss_size   = SIG_STACK_SIZE;
    stack.ss_flags  = 0;
    sigaltstack(&stack, (stack_t *)0);

    return 0;
}

/**
 * @brief 安装信号处理函数
 */
int install_signal_handler(const char *prefix, signal_cb cb, void *args)
{
    int ret;
    struct sigaction act;
    signal_cb_des_t cb_des;

    cb_des.cb   = cb;
    cb_des.args = args;

    ret = signal_stack_init(prefix, &cb_des);
    if (ret) {
        printf("[ERROR] change signal handler stack failed, ret [%d]\n", ret);
        return -1;
    }

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);
    act.sa_flags        = SA_SIGINFO | SA_ONSTACK;
    act.sa_sigaction    = &signal_handler;

    /* 注册信号处理函数 */
    sigaction(SIGSEGV, &act, NULL);
    sigaction(SIGBUS, &act, NULL);
    sigaction(SIGFPE, &act, NULL);
    sigaction(SIGILL, &act, NULL);

    return 0;
}




