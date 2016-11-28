
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
 *       Filename:  set_proc_center.cpp
 *
 *    Description:  上报逻辑处理中心
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  g++
 *
 *        Company:  Tencent
 *
 * =====================================================================================
 */

#include "set_proc_center.h"
#include "get_proc_center.h"
#include "judge_proc_center.h"
#include "log_def.h"
#include "proc_def.h"
#include "signal_handler.h"
#include "monitor.pb.h"
#include "data.pb.h"
#include "wbl_comm.h"

// 接收到信号立即退出程序
void Quit(int iSignal, void *pInfo)
{
	msg_log.Write("[Set]Receive signal(%u), program quiting...", iSignal);
	CGetProcCenter::GetInstance().Wait();
	CSetProcCenter::GetInstance().Wait();
	tce::CCommMgr::GetInstance().Stop();
	CJudgeProcCenter::GetInstance().Stop();
}

void SetPoolInit()	//线程池初始化
{
}

CSetProcCenter::CSetProcCenter(void) 
	: m_tCurTime(time(NULL))
	, m_iTcpID(0)
{}

bool CSetProcCenter::Init(void)
{
	try {
		//redis db
		mysql_redis.Init(stConfig.sRedisDBHost, stConfig.sRedisDBUser, stConfig.sRedisDBPassword, stConfig.wRedisDBPort);
		mysql_redis.use(stConfig.sRedisDBName);
	}catch(std::runtime_error& e)
	{
		cout << "Connect RedisDB Failed|" << e.what() << endl;
		return false;
	}
	CheckRedisInfo();

	// 设置定时器
	tce::CCommMgr::GetInstance().SetTimer(TT_CHECK_SIGNAL, stConfig.dwCheckSignalInterval);
	tce::CCommMgr::GetInstance().SetTimer(TT_PRINT_SHM_INFO, 10000);
	tce::CCommMgr::GetInstance().SetTimer(TT_CHECK_REDIS_INFO, stConfig.dwCheckRedisDBInterval * 1000);

	// 设置信号处理函数
	CSigHandler::GetInstance().SetSigHander(SIGTERM, Quit);

	//设置线程池处理
	m_pool.start(stConfig.dwThreadPoolSize, SetPoolInit);

	return true;
}

void CSetProcCenter::OnTimer(const int iId) 
{
	m_tCurTime = time(NULL);

	switch ( (TIMER_TYPE)iId )
	{
		case TT_CHECK_SIGNAL:
			CSigHandler::GetInstance().Check();
			break;
		case TT_PRINT_SHM_INFO:
			PrintShmInfo();
			break;
		case TT_CHECK_REDIS_INFO:
			CheckRedisInfo();
			break;
		default:
			err_log.Write("[Set]Undefined timer type(%u)", iId);
			break;
	}
}

void CSetProcCenter::PrintShmInfo()
{
	DataShm.PrintInfo("DATA");
	ServiceShm.PrintInfo("SERVICE");
}

void CSetProcCenter::CheckRedisInfo()
{		
	string sql = "select first_level_service_name, second_level_service_name, ip, port FROM t_service_info";
	std::tr1::shared_ptr<map<string, string> > host_servicenames_(new map<string, string>);
	try{
		const monitor::MySqlData& data = mysql_redis.query(sql);
		for(size_t i = 0; i < data.num_rows(); ++i){
			const monitor::MySqlRowData& r = data[i];
			host_servicenames_->insert(std::pair<string, string>(r["ip"]+":"+r["port"], r["first_level_service_name"]+"."+r["second_level_service_name"]));
		}
		host_servicenames = host_servicenames_;
		msg_log.Write("[Timer]RedisInfo|%u", host_servicenames->size());
	}
	catch(std::runtime_error& e)
	{
		err_log.Write("[Timer]CheckRedisInfo|ERR|SQL|%s", e.what());
	}
}

int CSetProcCenter::SendRespPkg(tce::SSession& stSession, uint32_t ret)
{
	msec::monitor::RespReport Pkg;
	Pkg.set_result(ret);

	string sData;
	if ( !Pkg.SerializeToString(&sData) )
	{
		err_log << "[Set]Failed to serialize to string!" << endl;
		return -2;
	}

	{
		
		tce::CAutoLock lock(lock_write);
		if ( !tce::CCommMgr::GetInstance().Write(stSession, sData.data(), sData.size()) )
		{
			err_log.Write("[Set]Send pkg back failed! ip: %s, port: %u", stSession.GetIPByStr().c_str(), stSession.GetPort());
			return -2;
		}
	}

	return 0;
}

int CSetProcCenter::UpdateValueData(const msec::monitor::Attr& attr, const string& host, const string& ip, int day, int begin, int end, int index, bool needjudge)	//from begin minute to end minute
{
	char data[value_fixed_len];

	uint32_t numeric_ip = 0;
	if(!ip.empty())
	{
		numeric_ip = tce::InetAtoN(ip);
		if(numeric_ip == 0)
		{
			err_log.Write("[Set]UpdateValueData IP input error|%s", ip.c_str());
			return -1;
		}
	}

	if(attr.servicename().size() > 127 || attr.attrname().size() > 127)
	{
			err_log.Write("[Set]UpdateValueData attr name input error|%lu|%lu", attr.servicename().size(), attr.attrname().size());
			return -1;
	}

	//KeyFormat: MD5(svc + 0x03 + attr) + [ dwIP ] + dwDay
	string key(attr.servicename());
	key.append(1u,0x3).append(attr.attrname());
	if(!host.empty())
		key.append(1u,0x3).append(host);
	string skey(tce::TC_MD5::md5bin(key));	//md5 + dayvalue
	if(!ip.empty())
		skey.append((char*)&numeric_ip, sizeof(uint32_t));
	skey.append((char*)&day, sizeof(uint32_t));

	int retry = 0;
	while(retry < 3)
	{
		int data_len = sizeof(data);
		int version = 0;
		int ret = DataShm.GetNode(skey, &data[0], data_len, &version);
		if(ret != 0)
		{
			err_log.Write("[Set]GetNode DataShm failed!|%s|%s|%s|%u|%d|%s", attr.servicename().c_str(), attr.attrname().c_str(), ip.c_str(), day, ret, DataShm.GetErrorMsg());
			return -2;
		}
		//这里的数据结构由于性能问题不采用pb
		if(data_len != value_fixed_len)	//直接初始化
		{
			memset(data, 0, value_fixed_len);
			strcpy(data, attr.servicename().c_str());
			strcpy(&data[128], attr.attrname().c_str());
		}
		else
		{
			
			if(strcmp(attr.servicename().c_str(), &data[0]) || strcmp(attr.attrname().c_str(), &data[128]))
			{				
				err_log.Write("[Set]Key not match|%s|%s|%s|%s|%s|%u", data, &data[128],
					attr.servicename().c_str(), attr.attrname().c_str(), ip.c_str(), day);
				return -4;
			}
		}

		//修改values
		char* ptr = &data[256];
		for(int j = begin; j <= end; j++)
		{
	//		int num = *(uint32_t*)(ptr+j*4);
			*(uint32_t*)(ptr+j*4) += attr.values(j-begin+index);

			if(needjudge && ip.empty())	//全量视图判断是否要转到告警
			{
				CJudgeProcCenter::GetInstance().AddJudge(attr.servicename(), attr.attrname(), day, j, *(uint32_t*)(ptr+j*4));
			}
		}
		//写入共享内存(value直接定长为256+1440*4)
		ret = DataShm.SetNode(skey, data, value_fixed_len, version);
		if(ret == -1000) //data collision
		{
			err_log.Write("[Set]Value Write Retry|%s|%s|%s|%s|%u|%d", attr.servicename().c_str(), attr.attrname().c_str(), host.c_str(), ip.c_str(), day, version);
			retry++;
			continue;
		}
		else if( ret != 0)
		{
			err_log.Write("[Set]SetNode DataShm failed!|%s|%s|%s|%s|%u|%d|%s", attr.servicename().c_str(), attr.attrname().c_str(), host.c_str(), ip.c_str(), day, ret, DataShm.GetErrorMsg());
			return -5;
		}
		else
		{
			//msg_log.Write("[Set]Value Write OK|%s|%s|%s|%s|%u|%d", attr.servicename().c_str(), attr.attrname().c_str(), host.c_str(), ip.c_str(), day, version);
			break;
		}
	}
	if(retry == 3)
	{
		err_log.Write("[Set]Value Write Retry Limit|%s|%s|%s|%s|%u", attr.servicename().c_str(), attr.attrname().c_str(), host.c_str(), ip.c_str(), day);
		return -6;		
	}
	return 0;
}

int CSetProcCenter::OnRead(tce::SSession& stSession, const unsigned char* pszData, const size_t nSize)
{
	// 解析接收的包
	std::tr1::shared_ptr<msec::monitor::ReqReport> Pkg(new msec::monitor::ReqReport());
	if ( !Pkg->ParseFromArray(pszData,nSize) )
	{
		err_log.Write("[Set]Parse from string failed! src info: (%s:%u)", stSession.GetIPByStr().c_str(), stSession.GetPort());
		return -1;
	}
//	msg_log.Write("Pkg|%s", Pkg->DebugString().c_str());
#if 0
	return Process(stSession, Pkg);
#else
	m_pool.execute_memfun(CSetProcCenter::GetInstance(), &CSetProcCenter::Process, wbl::ref(stSession), Pkg);
	return 0;
#endif
}

int CSetProcCenter::Process(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqReport> Pkg)
{
	string srcip = stSession.GetIPByStr();
	if(srcip == "127.0.0.1")
	{
		srcip = stConfig.sLocalIP;
	}
	int ret = ProcessReq(*Pkg.get(), srcip);
	return SendRespPkg(stSession, ret);
}

int CSetProcCenter::ProcessReq(msec::monitor::ReqReport& Pkg, string& srcip, bool timecheck)
{
	uint32_t ip = tce::InetAtoN(srcip);
	tce::CTimeCost tc;
	char data[128 * 1024];
	int data_len = sizeof(data);

	bool judge_check = (stConfig.bAlarm || timecheck);	//只要有任何一个为true则不走judge逻辑

	//先查IP
	string key( (char*)&ip, sizeof(uint32_t));
	int ret = ServiceShm.GetNode(key, &data[0], data_len);
	if(ret != 0)
	{
		err_log.Write("[Set]GetNode ServiceShm failed!|%s|%d|%s", srcip.c_str(), ret, ServiceShm.GetErrorMsg());
		return 1;
	}
	
	msec::monitor::data::IPData ipdata;		//IP - 属性数据
	map<string, set<string> > service_attr_ipmem;	//内存中的IP - ServiceName - 属性数据
	map<string, set<string> > service_attr_mem;	//内存中的ServiceName - 属性数据
	map<string, set<string> > service_attr_req;	//请求包中的ServiceName - 属性数据

	map<string, set<uint64_t> > service_hosts;
	if(data_len > 0 &&  !ipdata.ParseFromArray(data, data_len) )
	{
		err_log.Write("Parse from ServiceShm failed!|%s", srcip.c_str());
		return 2;
	}

	for(int i = 0; i < ipdata.services_size(); i++)
	{
		for(int j = 0; j < ipdata.services(i).attrs_size(); j++)
		{
			service_attr_ipmem[ipdata.services(i).servicename()].insert(ipdata.services(i).attrs(j).name());
		}
	}

	time_t tnow(time(NULL));
	for(int i = 0; i < Pkg.attrs_size(); i++)
	{
		//get set time series data...
		uint32_t begin = Pkg.attrs(i).begin_time();
		uint32_t end = Pkg.attrs(i).end_time();
		
		if ((int)((end-begin)/60+1) != Pkg.attrs(i).values_size())	
		{
			err_log.Write("[Set]Monitor Report value size error|%s|%u|%u|%lu|%d", srcip.c_str(), begin, end, tnow, Pkg.attrs(i).values_size());
			continue;
		}

		if(timecheck && (tnow - begin > 90000 || end - begin >= 86400))  //超过一天的数据略过
		{
			err_log.Write("[Set]Monitor Report time interval error|%s|%u|%u|%lu|%d", srcip.c_str(), begin, end, tnow, Pkg.attrs(i).values_size());
			continue;
		}

		//set..
		tce::CTimeAnalysor ta_begin(begin);
		int day = ta_begin.GetDayValue();
		int ta_begin_min = ta_begin.GetMinIdxOfDay();
		int ta_end_min = ta_begin_min + Pkg.attrs(i).values_size() - 1;
		bool NextDay = false;
		if(ta_end_min >= 1440)
		{
			ta_end_min = 1439;	//last minute
			NextDay = true;
		}

		string host = "";
		string hostip = "";
		uint16_t hostport = 0;
		
		if(Pkg.attrs(i).servicename() != "RESERVED.monitor")
		{
			//需要依据IP做一个转换，如转换不成功则不写入
			if(timecheck)
			{
				map<string, string>::iterator it = host_servicenames->find(Pkg.attrs(i).servicename());
				if(it != host_servicenames->end()) {
					host = Pkg.attrs(i).servicename();
					hostip = host.substr(0, host.find(":"));
					hostport = wbl::s2u(host.substr(host.find(":")+1));
					service_hosts[it->second].insert( ((uint64_t)tce::InetAtoN(hostip)<<16)+hostport );
					Pkg.mutable_attrs(i)->set_servicename(it->second);
					
				}
				else {
					err_log.Write("Unrecognized service|%s", Pkg.attrs(i).servicename().c_str());
					continue;
				}
			}
				
			service_attr_req[Pkg.attrs(i).servicename()].insert(Pkg.attrs(i).attrname());
			UpdateValueData(Pkg.attrs(i), "", "", day, ta_begin_min, ta_end_min, 0, judge_check);	//view	
			UpdateValueData(Pkg.attrs(i), host, hostip, day, ta_begin_min, ta_end_min, 0, judge_check);	//ip:port
		}
		else
		{
			//仅需要更新ip对应属性
			service_attr_req[Pkg.attrs(i).servicename()].insert(Pkg.attrs(i).attrname());
			UpdateValueData(Pkg.attrs(i), "", srcip, day, ta_begin_min, ta_end_min, 0, judge_check);	//ip		
		}
		
		if(NextDay)
		{
			int index = ta_end_min - ta_begin_min + 1;
			ta_end_min = ta_begin_min + Pkg.attrs(i).values_size() - 1441;
			ta_begin_min = 0;
			ta_begin.SetTime(end);
			day = ta_begin.GetDayValue();
			if(Pkg.attrs(i).servicename() != "RESERVED.monitor")
			{
				UpdateValueData(Pkg.attrs(i), "", "", day, ta_begin_min, ta_end_min, index, judge_check);	//view
				UpdateValueData(Pkg.attrs(i), host, hostip, day, ta_begin_min, ta_end_min, index, judge_check);	//ip:port
			}
			else
			{
				//仅需要更新ip对应属性
				UpdateValueData(Pkg.attrs(i), "", srcip, day, ta_begin_min, ta_end_min, index, judge_check);	//ip
			}
//			UpdateValueData(Pkg.attrs(i), "", day, ta_begin_min, ta_end_min, index);	//view
//			UpdateValueData(Pkg.attrs(i), srcip, day, ta_begin_min, ta_end_min, index);	//ip
		}
	}


	//IP相关只更新RESERVED.monitor
	//判断是否要更新IP
	bool WriteIP = false;
//	for(map<string, set<string> >::iterator it  = service_attr_req.begin(); it != service_attr_req.end(); it++)		

	map<string, set<string> >::iterator itsa  = service_attr_req.find("RESERVED.monitor");
	if(itsa!=service_attr_req.end())
	{
		map<string, set<string> >::iterator itfind = service_attr_ipmem.find(itsa->first);
		if(itfind == service_attr_ipmem.end())
		{
			WriteIP = true;
			service_attr_ipmem[itsa->first] = itsa->second;
		}
		else
		{
			size_t num = itfind->second.size();
			itfind->second.insert(itsa->second.begin(), itsa->second.end());
			if(itfind->second.size() != num)
				WriteIP = true;
		}

		if(WriteIP) {
			msec::monitor::data::IPData ipdata_new;		//IP - 属性数据	
			for(map<string, set<string> >::iterator it  = service_attr_ipmem.begin(); it != service_attr_ipmem.end(); it++)
			{
				msec::monitor::data::ServiceData* service = ipdata_new.add_services();
				service->set_servicename(it->first);
				for(set<string>::iterator itattr = it->second.begin(); itattr != it->second.end(); itattr++)
				{
					msec::monitor::data::Attr* attr = service->add_attrs();
					attr->set_name(*itattr);
				}			
			}
			string out;
			if ( !ipdata_new.SerializeToString(&out) )
			{
				err_log << "[Set]Failed to serialize to string!|" << srcip << endl;			
			}
			else
			{
				ret = ServiceShm.SetNode(key, out);	//no need to add ip
				if( ret != 0)
				{
					err_log.Write("[Set]SetNode ServiceShm failed!|%s|%d|%s", srcip.c_str(), ret, ServiceShm.GetErrorMsg());
				}
				else
				{
					msg_log.Write("[Set]IP Write OK|%s|%u", srcip.c_str(), service_attr_ipmem.size());
				}
			}
		}
	}

	
	//写入Service
	for(map<string, set<string> >::iterator it = service_attr_req.begin(); it != service_attr_req.end(); it++)
	{
		if(it->first == "RESERVED.monitor")
			continue;
		msec::monitor::data::ServiceData servicedata;		//Service - 属性数据
		string skey = tce::TC_MD5::md5bin(it->first);
		data_len = sizeof(data);
		int ret = ServiceShm.GetNode(skey, &data[0], data_len);
		if(ret != 0)
		{
			err_log.Write("[Set]GetNode ServiceShm failed!|%s", it->first.c_str());
			continue;
		}
		if(data_len > 0)
		{
			if(!servicedata.ParseFromArray(data, data_len))
			{
				err_log.Write("[Set]Parse from ServiceShm failed!|%s", it->first.c_str());
				continue;
			}
			//如果有数据则再次检查是否一致
			if(servicedata.servicename() != it->first)
			{
				err_log.Write("[Set]ServiceShm key not match!|%s|%s", it->first.c_str(), servicedata.servicename().c_str());
				continue;
			}
		}

		//检查是否一致
		set<string> attr_set;
		for(int i =0; i < servicedata.attrs_size(); i++)
		{
			attr_set.insert(servicedata.attrs(i).name());	
		}
		size_t attr_size = attr_set.size();
		attr_set.insert(it->second.begin(), it->second.end());
		
		set<uint64_t> host_set;
		for(int i = 0; i < servicedata.ips_size(); i++)
		{
			for(int j = 0; j < servicedata.ips(i).ports_size(); j++)
				host_set.insert(((uint64_t)servicedata.ips(i).ip() << 16) + servicedata.ips(i).ports(j));
		}
		size_t host_size = host_set.size();

		map<string, set<uint64_t> >::iterator ithosts = service_hosts.find(it->first);
		if(ithosts != service_hosts.end())
			host_set.insert(ithosts->second.begin(), ithosts->second.end());
		if(attr_set.size() != attr_size || host_set.size() != host_size)
		{
			//写入
			msec::monitor::data::ServiceData servicedata_new;
			servicedata_new.set_servicename(it->first);
			for(set<string>::iterator itset = attr_set.begin(); itset != attr_set.end(); itset++)
			{
				msec::monitor::data::Attr* attr = servicedata_new.add_attrs();
				attr->set_name(*itset);
			}
			uint32_t ip = 0;
			msec::monitor::data::IP* pip;
			for(set<uint64_t>::iterator itset = host_set.begin(); itset != host_set.end(); itset++)
			{
				if(*itset >> 16 == ip) {
					pip->add_ports(*itset & 0x000000000000FFFF);
				}
				else
				{
					ip = *itset >> 16;
					pip = servicedata_new.add_ips();
					pip->set_ip(ip);
					pip->add_ports(*itset & 0x000000000000FFFF);
				}
			}
			string out;
			if ( !servicedata_new.SerializeToString(&out) )
			{
				err_log << "[Set]Failed to serialize to string!|" << it->first << endl;			
			}
			else
			{
				ret = ServiceShm.SetServiceNode(it->first, skey, out);
				if( ret != 0)
				{
					err_log.Write("[Set]SetNode ServiceShm failed!|%s", it->first.c_str());
				}
				else
				{
					msg_log.Write("[Set]Service Write OK|%s|%u|%u", it->first.c_str(), attr_set.size(), host_set.size());
				}
			}
		}
	}
	msg_log.Write("[Set]OK|%s|%u|%lu", srcip.c_str(), Pkg.attrs_size(), tc.value());
	return 0;
}

void CSetProcCenter::OnClose(tce::SSession& stSession)
{
	m_tCurTime = time(NULL);
	msg_log.Write("[Set]OnClose! ip: %s, port: %u", stSession.GetIP().c_str(), stSession.GetPort());
}

void CSetProcCenter::OnConnect(tce::SSession& stSession, const bool bConnectOk)
{
	m_tCurTime = time(NULL);
	if ( bConnectOk )
	{
		msg_log.Write("[Set]OnConnect! ip: %s, port: %u", stSession.GetIP().c_str(), stSession.GetPort());
	}
}

void CSetProcCenter::OnError(tce::SSession& stSession, const int32_t iErrCode, const char* pszErrMsg)
{
	m_tCurTime = time(NULL);
	err_log.Write("[err]OnError! comm_id: %u, err_code: %u, ip: %s, err_msg: %s", stSession.GetCommID(), iErrCode,
				stSession.GetIP().c_str(), pszErrMsg?pszErrMsg:"");
}
