
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


#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>

#define C_TM_PRIVATE_SIZE sizeof(struct timeval)
#define C_PUB_HEAD_SIZE  8
#define C_HEAD_SIZE  (C_PUB_HEAD_SIZE+C_TM_PRIVATE_SIZE)

void shmread(void *p, int psize,unsigned long print_interval)
{
    unsigned long out_interval; //milliseconds 
    volatile unsigned *head;
    volatile unsigned *tail;
    char *block;
    typedef struct { volatile int counter; } atomic_t;
    typedef struct qstatinfo
    {
        atomic_t msg_count;
        atomic_t process_count;
        atomic_t flag;
        atomic_t curflow;
    } Q_STATINFO;
    
    //init
    head = 0;
    tail = 0;
    block = 0;
    if(print_interval == 0)
    {
        out_interval = 50;
    }
    else
    {
        out_interval = print_interval;
    }
    
    p = (void*)((unsigned long)p+sizeof(Q_STATINFO));
    //the head of data section
    head = (unsigned*)p;
    //the tail of data section
    tail = head + 1;
    // pid field
    pid_t *pid = (pid_t *)(tail + 1);
    //data section base address
    block = (char*)(pid+ 1);
    
    Q_STATINFO* pstat =(Q_STATINFO*) ((char*)p -sizeof(Q_STATINFO));
    while(1)
    {
        printf("*************************************\n");
        printf("head:\t%u\ttail:\t%u\tpid:\t%d\tblock:\t%p\n", 
                *head,
                *tail,
                *pid,
                block);
        printf("msg_count:\t%d\tprocess_count:\t%d\tflag:\t%d\tcurflow:\t%d\n",
                pstat->msg_count,
                pstat->process_count,
                pstat->flag,
                pstat->curflow);
        usleep(out_interval*1000);
    }
}

int main(int argc, char *argv[])
{
    unsigned key;
    int shmid;
    const int shmsize = 0; //refer to spp_workerN.xml send_size = ? and recv_size = ?
    void *pshm;
    unsigned long interval;
    
    interval = 0;
    if(argc==2 || argc==3)
    {
        if(argc == 3)
        {
            interval = atol(argv[2]);
        }
        else
        {
            interval == 0;
        }
        printf("print interval:%u\n", interval);
    }
    else
    {
        printf("Useage: shmread shmkey [print_interval(milliseconds)]\n");
        exit(0);
    }
    
    key = 0;
    shmid = 0;
    pshm = NULL;
    
    key = strtoul(argv[1], NULL, 16); // 以16进制转换

    shmid = shmget(key, shmsize, 0);
    if(shmid == -1)
    {
        printf("shmget error, key = %d\t%m\n", key);
        return 0;
    }
    pshm = (void *)shmat(shmid, NULL, SHM_RDONLY);
    
    /* add shm logic here */
    shmread(pshm, shmsize, interval);

    /* add shm logic above */
    
    if(shmdt(pshm) == -1)
    {
        printf("detach error.\t%m\n");
    }
    
    return 0;
}
