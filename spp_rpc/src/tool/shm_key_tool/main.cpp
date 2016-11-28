
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
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  04/12/2009 06:41:09 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <libgen.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "keygen.h"
#include "config.h"

#define COLOR_GREEN "\e[1m\e[32m"
#define COLOR_RED   "\e[1m\e[31m"
#define COLOR_END   "\e[m"

using namespace std;
using namespace spp::comm;

int main(int argc , char** argv)
{
    if (argc < 2) {
        printf("usage:%s ../etc/config.ini\n", argv[0]);
        exit(1);
    }

    printf("%s%s%s", COLOR_RED, "\n!!!!注意：这个工具是用目录来计算shm key和mq key,所以必须在bin目录下运行，否则取到的key是不正确的!\n", COLOR_END);
    printf("%s%s%s", COLOR_RED, "!!!!ATTENTION:This tool is used to calc shm key and mq key by CWD, \nso it must be run under diractory \"bin\",or the key is no significance!\n\n", COLOR_END);
    //	system((char*)"rm -rf /tmp/mq_*.lock");
    unsigned mqkey = pwdtok(255);
    
    printf("MQ_KEY%s[0x%x]%s\n", COLOR_GREEN, mqkey, COLOR_END);

    ConfigLoader loader;
    if (loader.Init(argv[1]) != 0)
    {
        exit(-1);
    }

    Config& config  = loader.GetConfig();

    int id = 1;
    unsigned k1 = pwdtok(2 * id);
    unsigned k2 = pwdtok(2 * id + 1);
    printf("worker%s[%d]%s RECV_KEY%s[0x%x]%s SEND_KEY%s[0x%x]%s \n", 
           COLOR_GREEN, id, COLOR_END, COLOR_GREEN, k1, COLOR_END, COLOR_GREEN, k2, COLOR_END);

    printf("\n");
    return 0;
}

