
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


/**
 * @file mt_alone.cpp
 * @info 微线程单独使用事例
 */

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "syncincl.h"

#define  REMOTE_IP      "127.0.0.1"
#define  REMOTE_PORT    9988

// Task事例类:使用UDP单发单收接口
class UdpSndRcvTask
    : public IMtTask
{
public:
    virtual int Process() {
        // 获取目的地址信息, 简单示例
        static struct sockaddr_in server_addr;
        static int initflg = 0;

        if (!initflg) {
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = inet_addr(REMOTE_IP);
            server_addr.sin_port = htons(REMOTE_PORT);
            initflg = 1;
        }

        char buff[1024] = "This is a udp sendrecv task example!";
        int  max_len = sizeof(buff);
        
        int ret = mt_udpsendrcv(&server_addr, (void*)buff, 100, buff, max_len, 100);
        if (ret < 0)
        {
            printf("mt_udpsendrcv failed, ret %d\n", ret);
            return -1;
        }

        return 0;
    };
};


#define PKG_LEN  100
// 检查报文是否接受完成
int CheckPkgLen(void *buf, int len) {
    if (len < PKG_LEN)
    {
        return 0;
    }

    return PKG_LEN;
}

// Task事例类，使用TCP连接池单发单收接口
class TcpSndRcvTask
    : public IMtTask
{
public:
    virtual int Process() {
        // 获取目的地址信息, 简单示例
        static struct sockaddr_in server_addr;
        static int initflg = 0;

        if (!initflg) {
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = inet_addr(REMOTE_IP);
            server_addr.sin_port = htons(REMOTE_PORT);
            initflg = 1;
        }

        char buff[1024] = "This is a tcp sendrecv task example!";
        int  max_len = sizeof(buff);
        
        int ret = mt_tcpsendrcv(&server_addr, (void*)buff, 100, buff, max_len, 100, CheckPkgLen);
        if (ret < 0)
        {
            printf("mt_udpsendrcv failed, ret %d\n", ret);
            return -1;
        }

        return 0;
    };
};

// Task事例类: 业务可以用来验证微线程API可用性
class ApiVerifyTask
    : public IMtTask
{
public:
    virtual int Process() {

        // 调用业务使用微线程API
	printf("This is the api verify task!!!\n");

        return 0;
    };
};

int main(void)
{
    // 初始化微线程框架
    bool init_ok = mt_init_frame();
    if (!init_ok)
    {
        fprintf(stderr, "init micro thread frame failed.\n");
        return -1;
    }

    // 触发微线程切换
    mt_sleep(0);

    UdpSndRcvTask task1;
    TcpSndRcvTask task2;
    ApiVerifyTask task3;

    // 现在原生线程已经在demon的调度中了
    while (true)
    { // 这里示例一个并发操作
        IMtTaskList task_list;
        task_list.push_back(&task1);
        task_list.push_back(&task2);
        task_list.push_back(&task3);

        int ret = mt_exec_all_task(task_list);
        if (ret < 0)
        {
            fprintf(stderr, "execult tasks failed, ret:%d", ret);
            return -2;
        }

        // 循环检查每一个task是否执行成功，即Process的返回值
        for (unsigned int i = 0; i < task_list.size(); i++)
        {
            IMtTask *task = task_list[i];
            int result = task->GetResult();

            if (result < 0)
            {
                fprintf(stderr, "task(%u) failed, result:%d", i, result);
            }
        }

        // 睡眠500ms
        mt_sleep(500);
    }

    return 0;
}


