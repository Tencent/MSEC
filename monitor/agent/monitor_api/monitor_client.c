
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mmap_queue.h"
#include "monitor_client.h"

static struct mmap_queue *queue = NULL;

#pragma pack(1)
typedef struct {
	uint8_t Type;	//1: Add; 2: Set
	uint8_t ServiceNameLen;
	uint8_t AttrNameLen;
	uint32_t Value;
	char Data[1024];	//data
} QueueData;
#pragma pack()

//初始化
int Monitor_Init(const char* FileName)
{
	queue = mq_open(FileName);
	if(queue == NULL)
		return -1;
	return 0;
}

#define MAX_NAME_LEN 128
#define UTF8_LENGTH_1(uc)      (uc < 0x80)
#define UTF8_LENGTH_ERROR(uc)	 (uc < 0xC0)
#define UTF8_LENGTH_2(uc)      (uc < 0xE0)
#define UTF8_LENGTH_3(uc)      (uc < 0xF0)
#define UTF8_LENGTH_4(uc)      (uc < 0xF5)

#define UTF8_SUCCEED_CHAR(uc)  (uc >= 0x80 && uc < 0xC0)

//	1. 总存储长度不超过MAX_NAME_LEN
//   返回码:
//			0  正常
//			-1  名字为空
//			-2  名字里有不合法字符
//			-3  名字过长
int CheckName(const char* Name)
{
	if(Name[0] == '\0')	
	{
		return -1;	//名字为空
	}
	int left_len = MAX_NAME_LEN + 1; 	//with '\0'
	unsigned char *pCur = (unsigned char*)Name;
	unsigned char c1st = 0;
	unsigned char c2nd = 0;
	unsigned char c3rd = 0;
	unsigned char c4th = 0;

	while(left_len > 0 &&  (c1st=*pCur++))
	{
		--left_len;
		if (UTF8_LENGTH_1(c1st)) {
			if (0 == isprint(c1st))
				return -2;
			continue;
		}
		else if (UTF8_LENGTH_ERROR(c1st)) {
			return -2;
		}
		else if (UTF8_LENGTH_2(c1st)) {
			left_len -=1;
			c2nd = *pCur++;
			if(!UTF8_SUCCEED_CHAR(c2nd))
				return -2;
		}
		else if (UTF8_LENGTH_3(c1st)) {
			left_len -=2;
			c2nd = *pCur++;
			c3rd = *pCur++;
			if(!UTF8_SUCCEED_CHAR(c2nd) || !UTF8_SUCCEED_CHAR(c3rd))
				return -2;
		}
		else if (UTF8_LENGTH_4(c1st)) {
			left_len -=3;
			c2nd = *pCur++;
			c3rd = *pCur++;
			c4th = *pCur++;
			if(!UTF8_SUCCEED_CHAR(c2nd) || !UTF8_SUCCEED_CHAR(c3rd) || !UTF8_SUCCEED_CHAR(c4th))
				return -2;
		}		
		else
			return -2;	//名字里有不合法字符
	}
	if(left_len > 0)
		return 0;
	else
		return -3;//名字过长
}

#define AGENT_MMAP_FILE "/msec/agent/monitor/monitor.mmap"

// 累加上报的api
int Monitor_Add(const char* ServiceName, const char* AttrName, uint32_t Value)
{
	// 挂载mmap
	if ( NULL == queue )
	{
		if ( 0 != Monitor_Init(AGENT_MMAP_FILE) )
		{
			return -1;
		}
	}
	if(CheckName(ServiceName) || CheckName(AttrName))
		return -3;
	QueueData qd;
	qd.Type = 1;	//add
	qd.ServiceNameLen = strlen(ServiceName);	
	qd.AttrNameLen = strlen(AttrName);
	qd.Value = Value;
	int len = 7 + (int)qd.ServiceNameLen + (int)qd.AttrNameLen;
	memcpy(qd.Data, ServiceName, (int)qd.ServiceNameLen);
	memcpy(qd.Data+(int)qd.ServiceNameLen, AttrName, (int)qd.AttrNameLen);
	return mq_put(queue, (void*)&qd, len);
}
	
// 覆盖上报的api
int Monitor_Set(const char* ServiceName, const char* AttrName, uint32_t Value)
{	
	// 挂载mmap
	if ( NULL == queue )
	{
		if ( 0 != Monitor_Init(AGENT_MMAP_FILE) )
		{
			return -1;
		}
	}
	if(CheckName(ServiceName) || CheckName(AttrName))
		return -3;
	QueueData qd;
	qd.Type = 2;	//set
	qd.ServiceNameLen = strlen(ServiceName);	
	qd.AttrNameLen = strlen(AttrName);
	qd.Value = Value;
	int len = 7 + (int)qd.ServiceNameLen + (int)qd.AttrNameLen;
	memcpy(qd.Data, ServiceName, (int)qd.ServiceNameLen);
	memcpy(qd.Data+(int)qd.ServiceNameLen, AttrName, (int)qd.AttrNameLen);
	return mq_put(queue, (void*)&qd, len);
}

