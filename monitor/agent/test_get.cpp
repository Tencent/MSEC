
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
#include <stdio.h>
#include <signal.h>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include "wbl_config_file.h"
#include "wbl_comm.h"
#include "tc_clientsocket.h"
#include "monitor.pb.h"

using namespace std;
using namespace wbl;

int usage()
{
	printf("cmd type [params]\n");
	printf("Server IP set in ip.conf\n");
	printf("\ttype = 1: get service\n");
	printf("\t\tcmd 1 servicename (days)\n");
	printf("\ttype = 2: get serviceattr\n");
	printf("\t\tcmd 2 servicename attrname day\n");
	printf("\ttype = 3: get attrip\n");
	printf("\t\tcmd 3 servicename attrname ip day\n");
	printf("\ttype = 4: get ip info\n");
	printf("\t\tcmd 4 ip (days)\n");
	printf("\ttype = 5: get ipattr\n");
	printf("\t\tcmd 5 ip servicename attrname day\n");
	printf("\ttype = 10: get servicelist\n");
	printf("\t\tcmd 10\n");
	printf("\ttype = 20: set alarm attr\n");
	printf("\t\tcmd 20 servicename attrname max min diff diffp\n");
	printf("\ttype = 21: del alarm attr\n");
	printf("\t\tcmd 21 servicename attrname\n");
	printf("\ttype = 22: get newest alarms\n");
	printf("\t\tcmd 22 servicename day\n");	
	printf("\ttype = 23: del alarm\n");
	printf("\t\tcmd 23 servicename attrname alarmtype date time\n");
	return 0;
}

int main(int argc, char *argv[])
{
	CFileConfig config;
	config.Init("ip.conf");
	string ip = config["server\\IP"];
	uint32_t port = s2u(config["server\\GetPort"]);
	uint32_t timeout = s2u(config["server\\Timeout"]);

	if(ip.empty() || port==0 || timeout ==0) {
		printf("ip.conf error\n");
		return 0;
	}
	taf::TC_TCPClient client;
	client.init(ip, port, timeout);
//	sendrecv
	string req_str;
	msec::monitor::ReqMonitor req;
	if(argc < 2)
		return usage();	
	int cmd = atoi(argv[1]);
	if(cmd == 1)
	{
		if(argc < 3)
			return usage();
		msec::monitor::ReqService* service = req.mutable_service();
		service->set_servicename(argv[2]);
		for(int i = 3; i < argc; i++)
			service->add_days(atoi(argv[i]));
	}
	else if(cmd == 2)
	{
		if(argc != 5)
			return usage();
		msec::monitor::ReqServiceAttr* serviceattr = req.mutable_serviceattr();
		serviceattr->set_servicename(argv[2]);
		serviceattr->add_attrnames(argv[3]);
		serviceattr->add_days(atoi(argv[4]));
	}
	else if(cmd == 3)
	{
		if(argc != 6)
			return usage();
		msec::monitor::ReqAttrIP* attrip = req.mutable_attrip();
		attrip->set_servicename(argv[2]);
		attrip->set_attrname(argv[3]);
		attrip->add_ips(argv[4]);
		attrip->add_days(atoi(argv[5]));
	}
	else if(cmd == 4)
	{
		if(argc < 3)
			return usage();
		msec::monitor::ReqIP* ip = req.mutable_ip();
		ip->set_ip(argv[2]);
		for(int i = 3; i < argc; i++)
			ip->add_days(atoi(argv[i]));
	}
	else if(cmd == 5)
	{
		if(argc != 6)
			return usage();
		msec::monitor::ReqIPAttr* ipattr = req.mutable_ipattr();
		ipattr->set_ip(argv[2]);
		msec::monitor::IPData* ipdata = ipattr->add_attrs();
		ipdata->set_servicename(argv[3]);
		ipdata->add_attrnames(argv[4]);
		ipattr->add_days(atoi(argv[5]));
	}
	else if(cmd == 10)
	{
		if(argc != 2)
			return usage();
		//msec::monitor::ReqTreeList* treelist = req.mutable_treelist();
		req.mutable_treelist();
	}
	else if(cmd == 20)
	{
		if(argc != 8)
			return usage();
		msec::monitor::ReqSetAlarmAttr* setalarmattr = req.mutable_setalarmattr();
		setalarmattr->set_servicename(argv[2]);
		setalarmattr->set_attrname(argv[3]);
		if(atoi(argv[4]) != -1)
			setalarmattr->set_max(atoi(argv[4]));
		if(atoi(argv[5]) != -1)
			setalarmattr->set_min(atoi(argv[5]));
		if(atoi(argv[6]) != -1)
			setalarmattr->set_diff(atoi(argv[6]));
		if(atoi(argv[7]) != -1)
			setalarmattr->set_diff_percent(atoi(argv[7]));
	}
	else if(cmd == 21)
	{
		if(argc != 4)
			return usage();
		msec::monitor::ReqDelAlarmAttr* delalarmattr = req.mutable_delalarmattr();
		delalarmattr->set_servicename(argv[2]);
		delalarmattr->set_attrname(argv[3]);
	}
	else if(cmd == 22)
	{
		if(argc != 3 && argc != 4)
			return usage();
		msec::monitor::ReqNewestAlarm* newalarm = req.mutable_newalarm();
		if(argc == 3)
		{
			newalarm->set_day(atoi(argv[2]));
		}
		else
		{
			newalarm->set_servicename(argv[2]);
			newalarm->set_day(atoi(argv[3]));
		}
	}
	else if(cmd == 23)
	{
		if(argc != 7)
			return usage();
		msec::monitor::ReqDelAlarm* delalarm = req.mutable_delalarm();
		delalarm->set_servicename(argv[2]);
		delalarm->set_attrname(argv[3]);
		delalarm->set_type(msec::monitor::AlarmType(atoi(argv[4])));
		delalarm->set_day(atoi(argv[5]));
		delalarm->set_time(atoi(argv[6]));
	}
	else
		return usage();
	
	string body;
	if(!req.SerializeToString(&body))
	{
		printf("[error] fails to serialize.\n");
		return 0;
	}
	uint32_t size = htonl(body.size() + sizeof(uint32_t));
	req_str.assign((char*)&size, sizeof(uint32_t));
	req_str.append(body);
	static char recvBuf[128*1024];
	size_t len = sizeof(recvBuf);
	int ret = client.sendRecv(req_str.data(), req_str.size(), &recvBuf[0], len);
	if(ret == 0)
	{
		uint32_t pkg_len = ntohl(*(uint32_t*)&recvBuf[0]);
		if(pkg_len > 128*1024)
		{
			printf("pkg_len too long|%u\n", pkg_len);
			return 0;
		}
		printf("recv len:%lu|%u\n", len, pkg_len);
		if(len < pkg_len) {
			ret = client.recvLength(&recvBuf[len], pkg_len-len );
			if(ret == 0)
			{
				printf("recv completed.\n");
			}
			else
			{
				printf("sendrecv failed|%d\n", ret);
				return 0;
			}
		}
		msec::monitor::RespMonitor resp;
		if(resp.ParseFromArray( &recvBuf[4], pkg_len-4 ))
		{
			printf("%s\n",resp.ShortDebugString().c_str());
		}
	}
	return 0;
}

