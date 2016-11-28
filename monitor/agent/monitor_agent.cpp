
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


#include <arpa/inet.h>
#include <signal.h>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <iostream>

#include "mmap_queue.h"
#include "opt_time.h"
#include "wbl_config_file.h"
#include "wbl_timer.h"
#include "wbl_comm.h"
#include "tc_clientsocket.h"
#include "monitor.pb.h"

#include "nlbapi.h"

using namespace std;
using namespace taf;
using namespace wbl;

#define LB_MONITOR_ID "RESERVED.monitor"

#pragma pack(1)
typedef struct {
	uint8_t Type;	//1: Add; 2: Set
	uint8_t ServiceNameLen;
	uint8_t AttrNameLen;
	uint32_t Value;
	char Data[1024];	//data
} QueueData;
#pragma pack()

class AttrKey {
public:
	string ServiceName;
	string AttrName;
	uint8_t Type;
	bool operator < ( const AttrKey &rhs ) const
	{
		return ( ServiceName == rhs.ServiceName ? (AttrName < rhs.AttrName) : ( ServiceName < rhs.ServiceName ) );
	}
};

class SetKey {
public:
	string ServiceName;
	string AttrName;
	uint32_t MinTime;
	bool operator < ( const SetKey &rhs ) const
	{
		if(MinTime == rhs.MinTime)
			return ( ServiceName == rhs.ServiceName ? (AttrName < rhs.AttrName) : ( ServiceName < rhs.ServiceName ) );
		else
			return ( MinTime < rhs.MinTime );
	}
};

typedef struct {
	string ConfigFile;
	string MmapFile;
	uint32_t SemKey;
	uint32_t ElementSize;
	uint32_t ElementCount;
	uint32_t ReportInterval;
	uint32_t CollectInterval;
	string ServerIP;
	uint32_t ServerPort;
	uint32_t ServerTimeout;
	bool UseLB;
} CONFIG;

static struct mmap_queue *queue = NULL;
CONFIG stConfig;

map<AttrKey, map<uint32_t, uint32_t> > attr_map; 	//key - time_t(minutes) - values

set<SetKey> set_attr_set;	//存储上报过的Monitor_Set属性

timer t;	//schedule to run every second
taf::TC_TCPClient client;	//client for agent-server communication

static long timevaldiff(struct timeval* end, struct timeval* start)
{
	long msec;
	msec=(end->tv_sec-start->tv_sec)*1000;
	msec+=(end->tv_usec-start->tv_usec)/1000;
	return msec;
	 
}

static int CollectPkg()
{
	static char m[4096];
	struct timeval tv;
	struct timeval tnow;
	opt_gettimeofday(&tnow, NULL);

	int len = 0;
	bool Stop = false;
	while((len = mq_get(queue, m, sizeof(m), &tv))>0 && !Stop)
	{
		if(timevaldiff(&tnow, &tv) >= 86400000)	//超过1天的数据不上报了
		{
			continue;
		}
		else if(timevaldiff(&tnow, &tv) < 0) //超过的不继续了
		{
			Stop = true;
		}
		QueueData* qd = (QueueData*)m;
		AttrKey key;
		key.ServiceName.assign(qd->Data, qd->ServiceNameLen);
		key.AttrName.assign(qd->Data+qd->ServiceNameLen, qd->AttrNameLen);
		key.Type = qd->Type;
		uint32_t tmin = tv.tv_sec /60;
		if(qd->Type == 1)
			attr_map[key][tmin] += qd->Value;
		else if(qd->Type == 2)
		{
			
			SetKey skey;
			skey.ServiceName = key.ServiceName;
			skey.AttrName = key.AttrName;
			skey.MinTime = tmin;
			if(set_attr_set.find(skey) != set_attr_set.end())
			{
				//该属性已经在此分钟上报过了
				cout << "[" << t2s(tnow.tv_sec) << "] " << "Already Set|" << skey.ServiceName
					 << "|" << skey.AttrName << "|" << skey.MinTime << endl;
			}
			else
			{
				attr_map[key][tmin] = qd->Value;
				set_attr_set.insert(skey);
			}
		}
	}
	return 0;
}

time_t last_send_time = 0;

void SendPkg(time_t tnow)
{
	if(last_send_time == tnow)		//already sent..
		return;
	
	if(attr_map.size() == 0)
	{
		last_send_time = tnow;
		return;
	}

	map<string, int> report_stats;
//	cout << "[" << t2s(tnow) << "] " << "Report|" << attr_map.size() << endl;
	
	msec::monitor::ReqReport req;
	for(map<AttrKey, map<uint32_t, uint32_t> >::iterator it = attr_map.begin(); it !=attr_map.end(); it++)
	{
		report_stats[it->first.ServiceName]++;
		
		msec::monitor::Attr* attr = req.add_attrs();
		uint32_t begin = 0;
		uint32_t end = 0;
		attr->set_servicename(it->first.ServiceName);
		attr->set_attrname(it->first.AttrName);
		begin = it->second.begin()->first;
		end = it->second.rbegin()->first;
		for(uint32_t i = begin; i <= end; i++)
		{
			attr->add_values(0);
		}
		for(map<uint32_t, uint32_t>::iterator itmap = it->second.begin(); itmap != it->second.end(); itmap++)
		{
			int index = itmap->first - begin;
			attr->set_values(index, itmap->second);
		}
		attr->set_begin_time(begin * 60);
		attr->set_end_time(end * 60);
//		printf("Attr:%s|%s|%lu|%u|%u\n",it->first.ServiceName.c_str(), it->first.AttrName.c_str(), it->second.size(), begin, end);
	}

	for(map<string, int>::iterator it = report_stats.begin(); it!= report_stats.end(); it++)
	{
		cout << "[" << t2s(tnow) << "] " << "[REPORT] " << it->first << "|" << it->second << endl;
	}
	
	string req_str;
	string body;
	if(!req.SerializeToString(&body))
	{
		cout << "[" << t2s(tnow) << "] [ERR] " << "ReqReport fails to serialize." << endl;
		return;
	}
	uint32_t size = htonl(body.size() + sizeof(uint32_t));
	req_str.assign((char*)&size, sizeof(uint32_t));
	req_str.append(body);

//	sendrecv
	static char recvBuf[4096];
	size_t len = sizeof(recvBuf);
	int ret = client.sendRecv(req_str.data(), req_str.size(), &recvBuf[0], len);
#if 0	
	if (ret != 0)
	{
		ret = client.sendRecv(req_str.data(), req_str.size(), &recvBuf[0], len);
	}
#endif	
	if(ret == 0)
	{
		uint32_t recvlen =  *(uint32_t*)&recvBuf[0];
		msec::monitor::RespReport resp;
		if(len > 4 && len == ntohl(recvlen) && resp.ParseFromArray( &recvBuf[4], len-4 ))
		{			
			//成功后清除
			if(resp.result() == 0)
			{
				last_send_time = tnow;
				attr_map.clear();
			}
		}
		else
		{
			cout << "[" << t2s(tnow) << "] [ERR] " << "Parse resp|" << len << "|" << ntohl(recvlen) << endl;
		}
	}
	else
	{
		cout << "[" << t2s(tnow) << "] [ERR] " << "Report failed|" << ret << endl;
	}
	
}

void on_timer()
{
	CollectPkg();
	time_t now;
	opt_time(&now);

	if(stConfig.UseLB && now % 60 == 0)	//check ip:port every 60 seconds
	{
		//使用lb
		struct routeid id;
		int ret = getroutebyname(LB_MONITOR_ID, &id);
		if(ret)
		{
			printf("[ERR] get route error|%d\n", ret);
		}
		else
		{
			struct in_addr in;
			in.s_addr = id.ip;
			string newip = std::string(inet_ntoa(in));
			if(stConfig.ServerIP != newip || stConfig.ServerPort != id.port)
			{
				cout << "[CHG] GetRoute IP: " << stConfig.ServerIP << ":" << stConfig.ServerPort
					<< "->" << newip << ":" << id.port << endl;
				stConfig.ServerIP = newip;
				stConfig.ServerPort = id.port;
				client.init(stConfig.ServerIP, stConfig.ServerPort, stConfig.ServerTimeout);
			}
		}
	}
	
	if(now % stConfig.ReportInterval == 0)	//report
	{
		SendPkg(now);
		//Erase Set...
		set<SetKey>::iterator it = set_attr_set.begin();
		while(now/60 - it->MinTime >= 1440 && it!= set_attr_set.end())	//删除1天前的数据
		{
			it++;
		}
		set_attr_set.erase(set_attr_set.begin(), it);
	}
}

bool IsUniqueProc(int SemKey) 
{
	//use semaphore
	return true;
}


static int Init(int argc, char *argv[])
{
	CONFIG *pstConfig = &stConfig;
	if (argc < 2)
	{
		printf("Usage: %s config_file\n", argv[0]); 
		exit(0);
	}

	pstConfig->ConfigFile = argv[1];

	if ( pstConfig->ConfigFile == "-h" || pstConfig->ConfigFile == "-H" || pstConfig->ConfigFile == "?" )
	{
		printf("Usage: %s config_file\n", argv[0]);
		exit(0);
	}
	CFileConfig config;
	config.Init(pstConfig->ConfigFile);
	pstConfig->MmapFile = config["main\\MmapFile"];
	pstConfig->SemKey = s2u(config["main\\SemKey"]);
	pstConfig->ElementSize = s2u(config["main\\ElementSize"]);
	pstConfig->ElementCount = s2u(config["main\\ElementCount"]);
	pstConfig->ReportInterval = s2u(config["main\\ReportInterval"]);
	pstConfig->CollectInterval = s2u(config["main\\CollectInterval"]);
	pstConfig->UseLB = (s2u(config["main\\UseLB"]) == 1) ? true:false;
	pstConfig->ServerTimeout = s2u(config["server\\Timeout"]);

	if(!pstConfig->UseLB)
	{
		if(argc >= 3)
			pstConfig->ServerIP = argv[2];
		else			
			pstConfig->ServerIP = config["server\\IP"];
		
		pstConfig->ServerPort = s2u(config["server\\Port"]);
	}
	else
	{
		//使用lb
		struct routeid id;
		int ret = getroutebyname(LB_MONITOR_ID, &id);
		if(ret)
		{
			printf("[ERR] get route error|%d\n", ret);
			return -1;
		}
		struct in_addr in;
		in.s_addr = id.ip;
		pstConfig->ServerIP = std::string(inet_ntoa(in));
		pstConfig->ServerPort = id.port;
		cout << "[OK] GetRoute IP: " << pstConfig->ServerIP << ":" << pstConfig->ServerPort << endl;
	}

	if(pstConfig->SemKey == 0 || pstConfig->ElementSize == 0 || pstConfig->ElementCount == 0
		|| pstConfig->ReportInterval == 0 || pstConfig->CollectInterval == 0
		|| pstConfig->ServerIP == "" || pstConfig->ServerPort == 0 || pstConfig->ServerTimeout == 0)
	{
		printf("[ERR] config parameter error.\n");
		return -1;
	}

	queue = mq_create(pstConfig->MmapFile.c_str(), pstConfig->ElementSize, pstConfig->ElementCount);
	if(queue == NULL)
	{
		printf("[ERR] mmap queue create/open failed.\n");
		return -1;
	}

	//tcp_client
	client.init(pstConfig->ServerIP, pstConfig->ServerPort, pstConfig->ServerTimeout);
	return 0;
}


void DoLoop()
{
	t.schedule(make_fun_runnable(on_timer), stConfig.CollectInterval);	//every collectinterval ms
	while(1)
		sleep(1);
	return;
}

int main(int argc, char *argv[])
{
	if (Init(argc, argv)) 
    { 
        printf("[ERR] Initialize failed.\n");
        return -1; 
    }

	Daemon();

	DoLoop();

	return 0;
}

