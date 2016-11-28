
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


#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <libgen.h>

#include <sys/file.h>
#include <sys/types.h>
#include <dirent.h>

#include <errno.h>
#include <string>
#include <iostream>
#include "StatComDef.h"
using namespace std;
USING_SPP_STAT_NS;
#define COLOR_GREEN "\e[1m\e[32m"
#define COLOR_RED   "\e[1m\e[31m"
#define COLOR_END   "\e[m"

#define BEGIN_PRINT_GREEN printf("%s", COLOR_GREEN)
#define BEGIN_PRINT_RED   printf("%s", COLOR_RED)
#define END_COLOR         printf("%s\n", COLOR_END)
const char* COST_STAT_TOOL = "1.0.0";

int sleep_time = 5;
void usage()
{
    BEGIN_PRINT_RED;
    printf("%s",
       "\nUsage:-------------------------------------------------------------------------------------\n"
       "      1. ./cost_stat_tool -v                      查看工具版本信息                         \n"
       "      2. ./cost_stat_tool -t [group] [cmd]        查看以组,命令字为key的时延统计信息       \n"
       "      3. ./cost_stat_tool -t [group] [cmd] [code] 查看以组,命令字,返回码为key的时延统计信息\n"
       "      4. ./cost_stat_tool -r [group]              查看worker处理前端请求时延统计信息       \n"
       "      5. ./cost_stat_tool -l [group]              查看该组下所有命令字,以及返回码信息      \n"
       "      6. ./cost_stat_tool -c [group] [cmd]        查看该组该命令字下返回码实时次数统计信息 \n"
       "-------------------------------------------------------------------------------------------\n\n"
         );
    END_COLOR;

}
class CFileData
{
    public:
        CFileData(){_mappool = NULL; curIndex = 0;};
        ~CFileData()
        {
            if (_mappool == NULL)
            {
                munmap(_mappool, GET_MGR_POOL_SIZE);
                _mappool = NULL;
            }
        }
        int initMapPool(int group)
        {
            _mappool = NULL;
            char file_path[256] ;
            snprintf(file_path, sizeof(file_path) - 1, SPP_STAT_MGR_FILE, group);
            if (access(file_path, R_OK))
            {
                printf("can't access to the cost stat file[%s]\n", file_path);
                return -1;
            }
            int fd = ::open(file_path, O_RDONLY, 0666);
            if (fd < 0)
            {
                printf("can't open cost stat file[%s]\n", file_path);
                return -1;
            }
            _mappool = (StatMgrPool*) ::mmap(NULL, GET_MGR_POOL_SIZE, PROT_READ , MAP_SHARED, fd, 0);
            if ((MAP_FAILED ==_mappool ))
            {
                printf("can't mmap the cost stat file[%s] to mem: %m\n", file_path);
                ::close(fd);
                _mappool = NULL;
                return -1;
            }
            if (_mappool->_magic != STAT_MGR_MAGICNUM)
            {
                ::close(fd);
                ::munmap(_mappool, GET_MGR_POOL_SIZE);
                _mappool = NULL;
                printf ("cost stat file[%s]'s format error\n", file_path);
                return -1;
            }
            ::close(fd);
            curIndex = 0;
            _pool[0] = *(_mappool);
            return 0;

        }
        void updatePoolData()
        {
            curIndex = !curIndex;
            _pool[curIndex] = *(_mappool);
        }
        const StatMgrPool* getCurPoolData()
        {
            return &_pool[curIndex];
        }
        const StatMgrPool* getLastPoolData()
        {
            return &_pool[!curIndex];
        }
    private:
        StatMgrPool* _mappool;
        StatMgrPool  _pool[2];
        int curIndex;
};
CFileData g_data;
void showCmdCodeList()
{
    const StatMgrPool* pool = g_data.getCurPoolData();
    printf("---------------------------------------------------------\n");
    BEGIN_PRINT_GREEN;

    for (int i = 0; i < MAX_CMD_NUM; ++ i)
    {
        if (pool->_cmdobjs[i]._used == true)
        {
            printf("cmdid:%4d, description: [%s]\ncodelist:[", i, pool->_cmdobjs[i]._desc);
            bool first = 1;
            for (int j = 0; j < MAX_CODE_NUM; ++j)
            {
                if (pool->_cmdobjs[i]._codeobjs[j]._used == true)
                {
                    if (first == 1)
                    {
                        printf("%d", j);
                        first = 0;
                    }
                    else
                    {
                        printf(" ,%d", j);
                    }
                }
            }

            printf("]\n\n");
        }
    }
    END_COLOR;

    printf("--------------------------------------------------\n");
    return;
}
void showTimeMap(int* timemap)
{
    BEGIN_PRINT_GREEN;
    printf("0ms  ~ 19ms:   %d\n", timemap[0]);
    printf("20ms ~ 39ms:   %d\n", timemap[1]);
    printf("40ms ~ 59ms:   %d\n", timemap[2]);
    printf("60ms ~ 79ms:   %d\n", timemap[3]);
    printf("80ms ~ 99ms:   %d\n", timemap[4]);
    printf("100ms ~ 149ms: %d\n", timemap[5]);
    printf("150ms ~ 199ms: %d\n", timemap[6]);
    printf("200ms ~ 249ms: %d\n", timemap[7]);
    printf("250ms ~ 299ms: %d\n", timemap[8]);
    printf("300ms ~ 349ms: %d\n", timemap[9]);
    printf("350ms ~ 399ms: %d\n", timemap[10]);
    printf("400ms ~ 449ms: %d\n", timemap[11]);
    printf("450ms ~ 499ms: %d\n", timemap[12]);
    printf(">=500ms:       %d\n", timemap[13]);
    END_COLOR;
    return;
}
void diffTimeInfo(int cmdid, int code)
{
    int timemap[MAX_TIMEMAP_SIZE];
    int timecost = 0;
    int count = 0;
    g_data.updatePoolData();
    const StatMgrPool* newPool = g_data.getCurPoolData();
    const StatMgrPool* oldPool = g_data.getLastPoolData();
    memset(timemap, 0, sizeof(timemap));
    printf("------------------------interval = %d(s)--------------------------\n\n", sleep_time);
    int begin = 0;
    int end = MAX_CODE_NUM;
    if (code != -1)
    {
        begin = code;
        end = code + 1;
    }
    for (int i = begin; i < end; ++ i)
    {
        if (newPool->_cmdobjs[cmdid]._codeobjs[i]._used == true &&
                oldPool->_cmdobjs[cmdid]._codeobjs[i]._used == true)
        {
            for (int j = 0; j < MAX_TIMEMAP_SIZE; ++ j)
            {
                timemap[j] += (int)(STATMGR_VAL_READ(newPool->_cmdobjs[cmdid]._codeobjs[i]._timecost[j]) -
                        STATMGR_VAL_READ(oldPool->_cmdobjs[cmdid]._codeobjs[i]._timecost[j]));
            }
            count += (int)(STATMGR_VAL_READ(newPool->_cmdobjs[cmdid]._codeobjs[i]._count) -
                    STATMGR_VAL_READ(oldPool->_cmdobjs[cmdid]._codeobjs[i]._count));
            timecost += (int)(STATMGR_VAL_READ(newPool->_cmdobjs[cmdid]._codeobjs[i]._tmcostsum)
                    - STATMGR_VAL_READ(oldPool->_cmdobjs[cmdid]._codeobjs[i]._tmcostsum));
        }
    }

    showTimeMap(timemap);
    BEGIN_PRINT_RED;
    if (code == -1)
    {
        printf("total([cmdid=%d]):\n", cmdid);
    }
    else
    {
        printf("total([cmdid=%d][code=%d]):\n", cmdid, code);
    }
    printf("avg time(ms):  %d\n", count != 0? timecost/count: 0);
    printf("count:         %d\n", count);
    printf("total time(ms):%d\n", timecost);
    END_COLOR;

    return;
}
void showCmdTimeMap(int cmdid)
{
    const StatMgrPool* pool = g_data.getCurPoolData();
    if (pool->_cmdobjs[cmdid]._used == false)
    {
        printf("the cmdid = %d is not init\n", cmdid);
        return ;

    }
    while (1)
    {
        sleep(sleep_time);
        diffTimeInfo(cmdid, -1);

    }
    return;
}
void showCmdCodeTimeMap(int cmdid, int code)
{
    const StatMgrPool* pool = g_data.getCurPoolData();
    if (pool->_cmdobjs[cmdid]._used == false)
    {
        printf("the cmdid = %d is not init\n", cmdid);
        return ;

    }
    if (pool->_cmdobjs[cmdid]._codeobjs[code]._used == false)
    {
        printf("the cmdid = %d, code = %d is not used\n", cmdid, code);
        return;
    }
    while (1)
    {
        sleep(sleep_time);
        diffTimeInfo(cmdid, code);

    }
    return;
}
void showCodeCount(int cmdid)
{
    const StatMgrPool* pool = g_data.getCurPoolData();
    if (pool->_cmdobjs[cmdid]._used == false)
    {
        printf("the cmdid = %d is not init\n", cmdid);
        return ;
    }
    map<int, int> count;
    while (1)
    {
        printf("------------------------interval = %d(s)--------------------------\n\n", sleep_time);
        sleep(sleep_time);
        printf("cmdid[%d]'s code stat: \n", cmdid);
        count.clear();
        g_data.updatePoolData();
        const StatMgrPool* newPool = g_data.getCurPoolData();
        const StatMgrPool* oldPool = g_data.getLastPoolData();
        for (int i = 0; i < MAX_CODE_NUM; ++ i)
        {
            if (newPool->_cmdobjs[cmdid]._codeobjs[i]._used == true &&
                    oldPool->_cmdobjs[cmdid]._codeobjs[i]._used == true)
            {
                count[i] += STATMGR_VAL_READ(newPool->_cmdobjs[cmdid]._codeobjs[i]._count) -
                    STATMGR_VAL_READ(oldPool->_cmdobjs[cmdid]._codeobjs[i]._count);
            }
        }
        BEGIN_PRINT_GREEN;
        int total = 0;
        for (map<int, int>::iterator it = count.begin();
                it != count.end(); ++ it)
        {
            printf("code[%d]: %6d\n", it->first, it->second);
            total += it->second;
        }
        printf("total:    %6d\n", total);
        END_COLOR;
    }
    return;
}
int main(int argc, char* const argv[])
{
    if (argc < 2)
    {
        usage();
        return -1;
    }
    if(!strncmp(argv[1], "-v", 2)) {
        printf("srpc cost stat tool ver: %s\n", COST_STAT_TOOL);
        return 0;
    }

    if (argc < 3)
    {
        usage();
        return -1;
    }
    int group = 0;
    int cmdid = 0;
    int code = 0;
    group = atoi(argv[2]);
    if (group <= 0 || group >= 256)
    {
        printf("group must be range in [1, 255]\n");
        return -1;
    }
    int ret = g_data.initMapPool(group);

    if (ret != 0)
    {
        return -1;
    }
    if (!strncmp(argv[1], "-l", 2))
    {
        showCmdCodeList();
    }
    else if (!strncmp(argv[1], "-t", 2))
    {
        if (argc != 4 && argc != 5)
        {
            usage();
            return -1;
        }
        cmdid = atoi(argv[3]);
        if (cmdid < 0 || cmdid >= MAX_PLUGIN_CMD_NUM )
        {
            printf("cmdid must be range in[0, %d]\n", MAX_PLUGIN_CMD_NUM - 1);
            return -1;
        }
        if (argc == 4)
        {
            showCmdTimeMap(cmdid);
        }
        else if (argc == 5)
        {
            code = atoi(argv[4]);
            if (code < 0 || code >= MAX_CODE_NUM)
            {
                printf("code must be range in[0, %d]\n", MAX_CODE_NUM - 1);
                return -1;
            }
            showCmdCodeTimeMap(cmdid, code);
        }
    }
    else if (!strncmp(argv[1], "-r", 2))
    {
        if (argc != 3)
        {
            usage();
            return -1;
        }
       showCmdCodeTimeMap(FRAME_REQ_STAT_CMD_ID, FRAME_REQ_STAT_CODE_ID);
    }
    else if (!strncmp(argv[1], "-c", 2))
    {
        if (argc != 4)
        {
            usage();
            return -1;
        }
        cmdid = atoi(argv[3]);
        if (cmdid <0 || cmdid >= MAX_CMD_NUM)
        {
            printf("cmdid must be range in[0, %d]\n", MAX_CMD_NUM - 1);
            return -1;
        }
        showCodeCount(cmdid);
    }
    else
    {
        usage();
    }
    return 0;
}
