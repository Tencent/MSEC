
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
#include <string.h>
#include "log.h"
#include "sysinfo.h"

typedef unsigned long long   llu64_t;

/* CPU 实时快照结构定义 */
typedef struct _tag_cpu_stat
{
    uint64_t    total;          /* 总的计数值 */
    uint64_t    user;           /* 累计的 用户态 */
    uint64_t    nice;           /* 累计的 nice为负 */
    uint64_t    system;         /* 系统态运行时间 */
    uint64_t    idle;           /* IO等待以外的其他等待时间 */
    uint64_t    iowait;         /* IO等待时间 */
    uint64_t    irq;            /* 硬中断 */
    uint64_t    softirq;        /* 软中断 */
}cpu_stat_t;

/* 内存数据结构定义 */
typedef struct _tag_mem_stat
{
    uint64_t    mem_total;      /* 总的内存 KB  */
    uint64_t    mem_free;       /* 可用物理内存 */
    uint64_t    buffers;        /* buffer内存   */
    uint64_t    cached;         /* 文件cache等  */
    uint64_t    mapped;         /* 已映射内存   */
    uint64_t    inactive_file;  /* 不活跃的文件cache */
}mem_stat_t;

/* 进程cpu数据定义 */
typedef struct _tag_proc_cpu
{
    uint64_t    utime;          /* 用户态时间 */
    uint64_t    stime;          /* 系统态时间 */
    uint64_t    cutime;         /* 子线程用户态时间 */
    uint64_t    cstime;         /* 子线程系统态时间 */
    uint64_t    proc_total;     /* 进程汇总时间 */
    uint64_t    sys_total;      /* 系统切片时间 */
}proc_cpu_t;

/* 全局的系统资源信息 */
struct sysinfo
{
    uint64_t    cpu_idle;        /* cpu 空闲时间 */
    uint64_t    cpu_total;       /* cpu 总数 */
    uint64_t    mem_total;       /* 内存总数 KB  */
    uint64_t    mem_free;        /* 内存可用 KB  */
    cpu_stat_t  records[2];      /* cpu统计值, 2次取差值 */
};

/* 进程资源信息 */
struct procinfo
{
    uint32_t    pid;              /* 进程pid */
    uint64_t    mem_used;         /* 物理内存kb */
    uint32_t    cpu_used;         /* 比例相对单CPU比例 */
    proc_cpu_t  records[2];       /* 快照信息 */
};

static struct sysinfo sys_stat;

/**
 * @brief 提取CPU统计信息
 */            
static void extract_cpu_stat(cpu_stat_t *stat)
{
    char name[64];
    char line[512];
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) {
        NLOG_ERROR("open /proc/stat failed, [%m]");
        return;
    }
    memset(stat, 0, sizeof(*stat));

    while (fgets(line, sizeof(line)-1, fp)) {
        /* cpu  155328925 640355 14677305 9174748668 331430975 0 457328 7242581 */
        if (sscanf(line, "%s%llu%llu%llu%llu%llu%llu%llu", name, 
                   (llu64_t*)&stat->user, (llu64_t*)&stat->nice,
                   (llu64_t*)&stat->system, (llu64_t*)&stat->idle,
                   (llu64_t*)&stat->iowait, (llu64_t*)&stat->irq,
                   (llu64_t*)&stat->softirq) != 8)
        {
            continue;
        }

        /* 只获取CPU总统计数据 */
        if (!strncmp(name, "cpu", 4)) {
            stat->total = stat->user + stat->nice + stat->system + stat->idle 
                          + stat->iowait + stat->irq + stat->softirq;
            break;
        }
    }

    fclose(fp);

    return;
}

/**
 * @brief 提取内存统计信息
 */   
static int32_t extract_mem_info(mem_stat_t *stat)
{
    char name[256];
    char line[256];
    int32_t finish_num = 0;
    llu64_t value;
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        NLOG_ERROR("open /proc/meminfo failed, [%m]");
        return -1;
    }

    while (fgets(line, sizeof(line)-1, fp)) {
        if (sscanf(line, "%s%llu", name, &value) != 2) {
            continue;
        }

        if (!strcmp(name, "MemTotal:")) {
            finish_num++;
            stat->mem_total = (uint64_t)value;
        } else if (!strcmp(name, "MemFree:")) {
            finish_num++;
            stat->mem_free = (uint64_t)value;
        } else if (!strcmp(name, "Buffers:")) {
            finish_num++;
            stat->buffers = (uint64_t)value;
        } else if (!strcmp(name, "Cached:")) {
            finish_num++;
            stat->cached = (uint64_t)value;
        } /* else if (! strcmp(name, "Inactive:")) {
            finish_num++;
            stat->inactive_file = (uint64_t)value;
        }*/  else if (!strcmp(name, "Mapped:")) {
            finish_num++;
            stat->mapped = (uint64_t)value;
        }
    }

    fclose(fp);
    
    /* 确认所有字段都提取到了 */
    if (finish_num != 5) {
        NLOG_ERROR("extract /proc/meminfo failed!");
        return -2;
    }

    return 0;

}

/**
 * @brief 获取内存空闲大小
 */
static void calc_mem_free(struct sysinfo *info)
{
    mem_stat_t stat;

    extract_mem_info(&stat);
    info->mem_total = stat.mem_total;
    info->mem_free  = stat.mem_free + stat.buffers + stat.cached - stat.mapped;
}

/**
 * @brief 初始化系统信息
 */
void init_sysinfo(void)
{
    /* 获取CPU占用快照 */
    extract_cpu_stat(&sys_stat.records[0]);
    extract_cpu_stat(&sys_stat.records[1]);
    sys_stat.cpu_idle  = sys_stat.records[0].total;

    /* 获取内存信息 */
    calc_mem_free(&sys_stat);
}

/**
 * @brief 更新系统信息
 */
static void update_sysinfo(void)
{
    /* 获取CPU占用快照 */
    cpu_stat_t *last_stat = &sys_stat.records[0];
    cpu_stat_t *cur_stat  = &sys_stat.records[1];

    memcpy(last_stat, cur_stat, sizeof(cpu_stat_t));
    extract_cpu_stat(cur_stat);
    sys_stat.cpu_idle   = cur_stat->idle - last_stat->idle;
    sys_stat.cpu_total  = cur_stat->total - last_stat->total;

    /* 获取内存信息 */
    calc_mem_free(&sys_stat);
}

/**
 * @brief 获取系统信息
 * @info  CPU占用百分比，MEM总量，MEM空闲
 */
void get_sysinfo(uint32_t *load_percent, uint64_t *mem_total, uint64_t *mem_free)
{
    /* 更新当前系统信息 */
    update_sysinfo();

    /* CPU百分比 */
    *load_percent = 100 - (uint32_t)((100.0*sys_stat.cpu_idle)/sys_stat.cpu_total);

    /* 内存使用情况 */
    *mem_total    = sys_stat.mem_total;
    *mem_free     = sys_stat.mem_free;
}

