
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "vma.h"
#include "mem.h"
#include "memlog.h"

#define VMA_MAPS_PATH   "/proc/self/maps"

/**
 * @brief 分配内存
 */
void *heap_alloc(s_heap_t *heap, int size)
{
    if (heap) {
        return s_heap_alloc(heap, size);
    }
    
    return malloc(size);
}

/**
 * @brief 分配并初始化一个vma
 */
vma_t *vma_alloc(s_heap_t *heap)
{
    vma_t *vma;

    if (heap) {
        vma = (vma_t *)s_heap_alloc(heap, sizeof(vma_t));
    } else {
        vma = (vma_t *)malloc(sizeof(vma_t));
    }

    if (NULL == vma) {
        return NULL;
    }

    memset(vma, 0, sizeof(vma_t));

    return vma;
}

/**
 * @brief 读取fd里面的数据
 */
int vma_read(int fd, off_t off, char *buf, int size)
{
    int ret;
    int rlen = 0;

    ret = lseek(fd, off, SEEK_SET);
    if (ret == -1) {
        return -1;
    }

    do {
        ret = read(fd, buf + rlen, size - rlen);
        if (ret == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue; 
            }
            return -2;
        }

        if (ret == 0) {
            return rlen;
        }

        rlen += ret;
    } while (rlen != size);

    return rlen;
}

/**
 * @brief 检查[addr, addr+len)是否有相应的权限
 * @return  =1 有相应权限
 *          =0 没有权限
 */
int vma_check_prot(vma_list_t *list, void *addr, int len, int prot)
{
    vma_t *vma;
    char  *addr_start = (char *)addr;
    char  *addr_end   = (char *)addr + len;

    for (vma = list->head; vma != NULL; vma = (vma_t *)vma->next) {
        if ((addr_start >= (char *)vma->start)
            && (addr_end < (char *)vma->end)) {
            return ((vma->flags & prot) ? 1 : 0);
        }

        if (addr_end < (char *)vma->end) {
            return 0;
        }
    }

    return 0;
}

/**
 * @brief 返回地址所在的二进制文件
 */
const char *vma_get_bin_path(vma_list_t *list, void *addr)
{
    vma_t *vma;

    for (vma = list->head; vma != NULL; vma = (vma_t *)vma->next) {
        if (((char *)addr >= (char *)vma->start)
            && ((char *)addr < (char *)vma->end)) {
            return vma->file;
        }
    }

    return NULL;
}

/**
 * @brief 将vma加入vma_list_t中
 */
void vma_list_add(vma_list_t *list, vma_t *vma)
{
    vma->next = NULL;
    if (NULL == list->head) {
        list->head = vma;
        list->tail = vma;
        list->size = 1;
        return;
    }

    list->tail->next = vma;
    list->tail       = vma;
    list->size++;
}

char *str_chr(char *buf, int size, char c)
{
    int i;
    for (i = 0; i < size; i++) {
        if (buf[i] == c) {
            return buf + i;
        }
    }

    return NULL;
}

char *skip_space(char *str)
{
    while(*str == ' ')
        str++;
    return str;
}

/**
 * @brief 处理进程虚拟地址空间("/proc/self/maps"),并将信息加入链表中
 */
int vma_parse_line(char *buf, int size, s_heap_t *heap, vma_list_t *list)
{
    void *  addr_start;
    void *  addr_end;
    char *  dev;
    char *  file    = NULL;
    char *  pos;
    char *  npos;
    char    c;
    int     flags = 0;
    int     len;
    unsigned long ino;
    unsigned long off;

    char *buf_end;

    vma_t *vma;

    if (NULL == buf || size == 0 || NULL == list) {
        return -1;
    }

    // 起始地址
    buf_end     = buf + size;
    addr_start  = (void *)strtoul(buf, &npos, 16);
    if ((npos >= buf_end) || (*npos != '-')) {
        return -2;
    }

    // 结束地址
    pos = npos + 1;
    addr_end = (void *)strtoul(pos, &npos, 16);
    if ((npos >= buf_end) || (*npos != ' ')) {
        return -3;
    }

    // 标志位
    pos = skip_space(npos);
    while((c = *pos++)) {
        if ( c == 'r'){
            flags |= VMA_PROT_READ;
            continue;
        } else if (c == 'w') {
            flags |= VMA_PROT_WRITE;
            continue; 
        } else if (c == 'x') {
            flags |= VMA_PROT_EXEC;
            continue;
        } else if (c == 's') {
            flags |= VMA_PROT_SHARED;
            continue; 
        } else if (c == 'p') {
            flags |= VMA_PROT_PRIVATE;
            continue; 
        } else if (c == '-') {
            continue;
        } else {
            break;
        }
    }

    // offset
    pos = skip_space(pos);
    off = (unsigned long)strtoul(pos, &npos, 16);
    if ((npos >= buf_end) || (*npos != ' ')) {
        return -4;
    }
    
    // dev
    pos  = skip_space(npos);
    npos = str_chr(pos, buf_end - pos, ' ');
    if (NULL == npos || npos >= buf_end) {
        return -5;
    }

    len = npos - pos;
    dev = (char *)heap_alloc(heap, len+1);
    if (NULL == dev) {
        return -6;
    }

    strncpy(dev, pos, len);
    dev[len] = '\0';

    // ino
    pos = skip_space(npos);
    ino = (unsigned long)strtoul(pos, &npos, 10);

    // file path
    pos  = skip_space(npos);
    len  = buf_end - pos;
    if (len == 0) {
        goto RET;
    }

    file = (char *)heap_alloc(heap, len+1);
    if (NULL == file) {
        return -7;
    }

    strncpy(file, pos, len);
    file[len] = '\0';

RET:
    vma = vma_alloc(heap);
    if (NULL == vma) {
        return -8;
    }

    vma->start  = addr_start;
    vma->end    = addr_end;
    vma->flags  = flags;
    vma->offset = off;
    vma->file   = file;
    vma->ino    = ino;
    vma->dev    = dev;

    vma_list_add(list, vma);

    return 0;
}

/**
 * @brief 处理进程虚拟地址空间("/proc/self/maps")
 */
int vma_parse_lines(char *buf, int size, s_heap_t *heap, vma_list_t *list)
{
    int ret;
    int llen, plen = 0;
    char *  pos;

    do {
        pos = str_chr(buf + plen, size - plen, '\n');
        if (NULL == pos) {
            return plen;
        }

        llen = pos - buf - plen;
        ret = vma_parse_line(buf + plen, llen, heap, list);
        if (ret < 0) {
            return -1;
        }

        plen = plen + llen + 1;
    } while (plen != size);

    return plen;
}

/**
 * @brief 获取当前进程的虚拟地址空间信息
 * @info  为保证信号处理函数中能够安全调用，需要传入自定义的heap
 */
vma_list_t *vma_list_create(s_heap_t *heap)
{
    int     fd;
    int     rlen, plen, blen;
    off_t   off = 0;
    char    buff[1024];
    vma_list_t *vma_list;

    fd = open(VMA_MAPS_PATH, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }

    vma_list = heap_alloc(heap, sizeof(vma_list_t));
    if (NULL == vma_list) {
        close(fd);
        return NULL;
    }

    blen = sizeof(buff);
    while (1) {
        rlen = vma_read(fd, off, buff, blen);
        if (rlen < 0) {
            close(fd);
            return NULL;
        }

        if (rlen == 0) {
            close(fd);
            return vma_list;
        }

        plen = vma_parse_lines(buff, rlen, heap, vma_list);
        if (plen <= 0) {
            close(fd);
            return NULL;
        }

        off += plen;
    }

    return vma_list;
}

/**
 * @brief 将虚拟地址信息输出到fd，输出内容同("/proc/self/maps")
 */
int show_vma_list_fd(int fd, vma_list_t *list)
{
    int    ret;
    char   buf[1024];
    vma_t *vma;
    for (vma = list->head; vma != NULL; vma = (vma_t *)vma->next) {
        ret = snprintf(buf, sizeof(buf), "%lx-%lx %c%c%c%c %lx %s %lu %s\n",
                       (long)vma->start,
                       (long)vma->end,
                       vma->flags & VMA_PROT_READ ? 'r' : '-',
                       vma->flags & VMA_PROT_WRITE ? 'w' : '-',
                       vma->flags & VMA_PROT_EXEC ? 'x' : '-',
                       vma->flags & VMA_PROT_SHARED ? 's' : 'p',
                       (long)vma->offset,
                       vma->dev ? (char *)vma->dev : "",
                       vma->ino,
                       vma->file ? (char *)vma->file : ""
                       );
        write_all(fd, buf, ret);
    }

    return 0;
}

#if 0

int main()
{
    vma_list_t list = {0, 0, 0};
    char buf[100];
    snprintf(buf, 100, "cat /proc/%d/maps", getpid());
    system(buf);
    vma_list_create(&list);
    vma_list_show_fd(1, &list);

    return 0;
}

#endif

