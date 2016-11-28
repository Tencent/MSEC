
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
#include <sys/time.h>
#include "wbl_config_file.h"
#include "wbl_comm.h"
#include "tc_clientsocket.h"
#include "monitor.pb.h"

using namespace std;
using namespace wbl;

const uint32_t SEC_PER_MIN = 60;
const uint32_t MIN_PER_HOUR = 60;
const uint32_t HOUR_PER_DAY = 24;

const uint32_t SEC_PER_HOUR = MIN_PER_HOUR*SEC_PER_MIN;
const uint32_t SEC_PER_DAY = HOUR_PER_DAY*SEC_PER_HOUR;
const uint32_t MIN_PER_DAY = HOUR_PER_DAY*MIN_PER_HOUR;

class CLocalTime
{
public:
	CLocalTime(time_t tTime = ::time(NULL)): m_tTime(tTime)
	{
		localtime_r(&m_tTime, &m_stTM);
	}

	~CLocalTime()	{}

	time_t time(void) const
	{
		return m_tTime;
	}

	size_t hour(void) const
	{
		return m_stTM.tm_hour;
	}

	size_t minute(void) const
	{
		return m_stTM.tm_min;
	}

	size_t second(void) const
	{
		return m_stTM.tm_sec;
	}

private:
	time_t m_tTime;
	struct tm m_stTM;
};	


uint64_t GetTimeInUs()
{
	uint64_t ret;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ret = tv.tv_sec * (uint64_t)1000000 + tv.tv_usec;
	return ret;
}

int main(int argc, char *argv[])
{
	if(argc != 3 && argc != 4) {
		printf("Usage: cmd Servicename Attrname (value[default=1])\n");
		printf("Server IP set in ip.conf\n");
		return 0;
	}
	int value = 1;
	if(argc == 4)
		value = atoi(argv[3]);
	if(value <= 0)
		value = 1;
	
	CFileConfig config;
	config.Init("ip.conf");
	string ip = config["server\\IP"];
	uint32_t port = s2u(config["server\\SetPort"]);
	uint32_t timeout = s2u(config["server\\Timeout"]);

	if(ip.empty() || port==0 || timeout ==0) {
		printf("ip.conf error\n");
		return 0;
	}
	
	taf::TC_TCPClient client;
	client.init(ip, port, timeout);
//	sendrecv
	string req_str;
	msec::monitor::ReqReport req;

//    for (int i=0; i<1; ++i)
    {
        msec::monitor::Attr* attr = req.add_attrs();
        attr->set_servicename(argv[1]);
        attr->set_attrname(argv[2]);

        CLocalTime lt;
//        time_t day_begin_time = lt.time() - SEC_PER_HOUR*lt.hour()-SEC_PER_MIN*lt.minute()-lt.second();
//        attr->set_begin_time(day_begin_time - 60);
//        attr->set_end_time(day_begin_time + 60);
		time_t begin_time = lt.time() - lt.second() - 60;
		attr->set_begin_time(begin_time);
		attr->set_end_time(begin_time);
        attr->add_values(value);
    }
	
	
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

    uint64_t tbeg = GetTimeInUs();    
    int count = 1;
//    for (int i=0; i<count; ++i)
    {
        int ret = client.sendRecv(req_str.data(), req_str.size(), &recvBuf[0], len);
        if(ret == 0)
        {
            printf("recv len:%lu|%u\n", len, ntohl(*(uint32_t*)&recvBuf[0]));
            /*msec::monitor::RespReport resp;
            if(resp.ParseFromArray( &recvBuf[4], len-4 ))
            {
                printf("%s\n",resp.ShortDebugString().c_str());
            }*/
        }
        else
        {
            printf("sendrecv failed|%d\n", ret);
        }
    }
    uint64_t tend = GetTimeInUs();    
    printf("timecost: %lu ms\n", (tend - tbeg) / count / 1000);


	return 0;
}

