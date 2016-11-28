
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
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#include "mmap_queue.h"
//
// Below is a test program, please compile with:
// gcc -o mqtest -DSQ_FOR_TEST mmap_queue.c
//

static char m[1024*1024];

void test_put(struct mmap_queue *queue, int proc_count, int count, char *msg)
{
	int i;
	int pid = 0;

	for(i=1; i<=proc_count; i++)
	{
		if(fork()==0)
		{
			pid=i;
			break;
		}
	}

	for(i=0; pid && i<count; i++)
	{
		snprintf(m, 1024, "[%d:%d] %s", pid, i, msg);
		if(mq_put(queue, m, strlen(m))<0)
		{
			printf("put msg[%d] failed: %s\n", i, mq_errorstr(queue));
			return;
		}
	}
	if(pid) exit(0);
	while(wait(NULL)>0);
	printf("put successfully\n");
}

void test_get(struct mmap_queue *queue, int proc_count, int count)
{
	int i;
	int pid = 0;

	for(i=1; i<=proc_count; i++)
	{
		if(fork()==0)
		{
			pid=i;
			break;
		}
	}

	if(pid)
	{
		for(i=0; i<count; i++)
		{
			struct timeval tv;
			int l = mq_get(queue, m, sizeof(m), &tv);
			if(l<0)
			{
				printf("mq_get failed: %s\n", mq_errorstr(queue));
				break;
			}
			if(l==0)// no data
			{
				printf("no data\n");
				break;
			}
			// if we are able to retrieve data from queue, always
			// try it without sleeping
			m[l] = 0;
			printf("pid[%d] msg[%d] len[%d]: %s\n", pid, i, l, m);
		}
		exit(0);
	}
	while(wait(NULL)>0);
}

void press_test(struct mmap_queue *queue, uint32_t record_count, uint32_t record_size)
{
	struct timeval tv;
	int put_count=0,get_count=0;
	for(; put_count<record_count; )
	{
		while(put_count<record_count && mq_put(queue, m, record_size)==0) put_count ++;
		while(mq_get(queue, m, sizeof(m), &tv)>0) get_count ++;
	}
	printf("put %u, get %u finished\n", put_count, get_count);
}

int main(int argc, char *argv[])
{
	struct mmap_queue *queue;

	if(argc<3)
	{
badarg:
		printf("usage: \n");
		printf("     %s open <file>\n", argv[0]);
		printf("     %s create <file> <element_size> <element_count>\n", argv[0]);
		printf("     %s press <file> <record_count> <record_size>\n", argv[0]);
		printf("\n");
		return -1;
	}

	if(strcmp(argv[1], "open")==0 || strcmp(argv[1], "press")==0)
	{
		queue = mq_open(argv[2]);
	}
	else if(strcmp(argv[1], "create")==0 && argc==5)
	{
		queue = mq_create(argv[2], strtoul(argv[3], NULL, 10), strtoul(argv[4], NULL, 10));
	}
	else
	{
		goto badarg;
	}

	if(queue==NULL)
	{
		printf("failed to open mmap queue: %s\n", mq_errorstr(NULL));
		return -1;
	}
	else
	{
		printf("ele_size=%d, ele_count=%d\n", queue->head->ele_size, queue->head->ele_count);
	}

	if(strcmp(argv[1], "press")==0)
	{
		if(argc!=5) goto badarg;
		press_test(queue, strtoul(argv[3], NULL, 10), strtoul(argv[4], NULL, 10));
		return 0;
	}

	while(1)
	{
		static char cmd[1024*1024];
		printf("available commands: \n");
		printf("  put <concurrent_proc_count> <msg_count> <msg>\n");
		printf("  get <concurrent_proc_count> <msg_count>\n");
		printf("  quit\n");
		printf("cmd>"); fflush(stdout);
		if(fgets(cmd, sizeof(cmd), stdin)==NULL)
			return 0;
		if(strncmp(cmd, "put ", 4)==0)
		{
			char *pstr = cmd + 4;
			while(isspace(*pstr)) pstr ++;
			int proc_count = atoi(pstr);
			if(proc_count<1) proc_count = 1;
			while(isdigit(*pstr)) pstr ++;
			while(isspace(*pstr)) pstr ++;
			int count = atoi(pstr);
			if(count<1) count = 1;
			while(isdigit(*pstr)) pstr ++;
			while(isspace(*pstr)) pstr ++;
			test_put(queue, proc_count, count, pstr);
		}
		else if(strncmp(cmd, "get ", 4)==0)
		{
			char *pstr = cmd + 4;
			while(isspace(*pstr)) pstr ++;
			int proc_count = atoi(pstr);
			if(proc_count<1) proc_count = 1;
			while(isdigit(*pstr)) pstr ++;
			while(isspace(*pstr)) pstr ++;
			int count = atoi(pstr);
			if(count<1) count = 1;
			test_get(queue, proc_count, count);
		}
		else if(strncmp(cmd, "quit", 4)==0 || strncmp(cmd, "exit", 4)==0)
		{
			return 0;
		}
	}
	return 0;
}
