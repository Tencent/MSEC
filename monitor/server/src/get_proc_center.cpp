
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


#include "get_proc_center.h"
#include "judge_proc_center.h"

#include "log_def.h"
#include "proc_def.h"
#include "signal_handler.h"
#include "monitor.pb.h"
#include "data.pb.h"
#include "wbl_comm.h"

void GetPoolInit()	//线程池初始化
{
	thrData * data = new thrData();
	//DB初始化
	if(stConfig.bAlarm)
	{
		data->mysql_alarm.Init(stConfig.sAlarmDBHost, stConfig.sAlarmDBUser, stConfig.sAlarmDBPassword, stConfig.wAlarmDBPort);
		data->mysql_alarm.use(stConfig.sAlarmDBName);
	}
	wbl::thread_pool::set_thread_data(data, true); // 设置线程私有数据，第二个参数true表示线城池负责该数据的释放
}

CGetProcCenter::CGetProcCenter(void) 
	: m_tCurTime(time(NULL))
	, m_iTcpID(0)
{}

bool CGetProcCenter::Init(void)
{
	//设置线程池处理
	try{
		m_pool.start(stConfig.dwThreadPoolSize, GetPoolInit);
	}catch(std::runtime_error& e)
	{
		cout << "Connect AlarmDB Failed|" << e.what() << endl;
		return false;
	}
	return true;
}

int CGetProcCenter::SendRespPkg(tce::SSession& stSession, msec::monitor::RespMonitor& Pkg)
{
	string sData;
	if ( !Pkg.SerializeToString(&sData) )
	{
		err_log << "[Get]Failed to serialize to string!" << endl;
		return -2;
	}
	
	{
		tce::CAutoLock lock(lock_write);
		if ( !tce::CCommMgr::GetInstance().Write(stSession, sData.data(), sData.size()) )
		{
			err_log.Write("[Get]Send pkg back failed! ip: %s, port: %u", stSession.GetIPByStr().c_str(), stSession.GetPort());
			return -2;
		}
	}

	return 0;
}

int CGetProcCenter::GetValueData(const string& servicename, const string& attrname, const string& ip, uint32_t day, char* valuedata)
{
	char data[value_fixed_len];
	int data_len = sizeof(data);
	
	uint32_t numeric_ip = 0;
	if(!ip.empty())
	{
		numeric_ip = tce::InetAtoN(ip);
		if(numeric_ip == 0)
		{
			err_log.Write("[Get]GetValueData IP input error|%s", ip.c_str());
			return -1;
		}
	}
	
	string key(servicename); 
	key.append(1u,0x3).append(attrname);
	string skey(tce::TC_MD5::md5bin(key));	//md5 + dayvalue
	if(!ip.empty())
		skey.append((char*)&numeric_ip, sizeof(uint32_t));
	skey.append((char*)&day, sizeof(uint32_t));

	int ret = DataShm.GetNode(skey, &data[0], data_len);
	if(ret != 0)
	{
		err_log.Write("[Get]GetNode DataShm failed!|%s|%s|%s|%u|%d|%s", servicename.c_str(), attrname.c_str(), ip.c_str(), day, ret, DataShm.GetErrorMsg());
		return -2;
	}

//	msg_log.Write("[Get]GetNode %s|%s",  tce::HexShow2(key).c_str(),tce::HexShow(skey).c_str());
	
	if(data_len == value_fixed_len)
	{
		if(strcmp(&data[0], servicename.c_str()) || strcmp(&data[128],attrname.c_str()))
		{				
			err_log.Write("[Get]Key not match|%s|%s|%s|%s|%s|%u", &data[0], &data[128],
				servicename.c_str(), attrname.c_str(), ip.c_str(), day);
			return -4;
		}
		memcpy(valuedata, &data[256], 1440*4);
	}
	else
	{
		err_log.Write("[Get]GetValueData key error|%d|%s|%s|%s|%u", data_len, servicename.c_str(), attrname.c_str(), ip.c_str(), day);
		return -5;
	}
	return 0;
}

int CGetProcCenter::ProcService(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	int result = 100;
	ostringstream str;
	msec::monitor::RespMonitor resp;
	char data[128 * 1024];
	int data_len = sizeof(data);
	
	str << "Service|" << Pkg->service().servicename();
	string skey = tce::TC_MD5::md5bin(Pkg->service().servicename());
	msec::monitor::RespService* service = resp.mutable_service();
	//获取监控项，监控项可以脱离上报项存在
	CJudgeProcCenter::GetInstance().ProcServiceAlarmAttr(service, Pkg->service().servicename());
	str<< service->alarmattrs_size() << "|";

	
	int ret = ServiceShm.GetNode(skey, &data[0], data_len);
	if( ret != 0)
	{
		err_log.Write("[Get]GetNode ServiceShm failed!|%s|%d|%s", Pkg->service().servicename().c_str(), ret, ServiceShm.GetErrorMsg());
		result = 101;
	}
	else
	{
		msec::monitor::data::ServiceData servicedata;		//Service - 属性数据
		if(data_len > 0)
		{
			if(!servicedata.ParseFromArray(data, data_len))
			{
				err_log.Write("[Get]Parse from ServiceShm failed!|%s", Pkg->service().servicename().c_str());
				result = 102;
			}
			else
			{
				//如果有数据则再次检查是否一致
				if(servicedata.servicename() != Pkg->service().servicename())
				{
					err_log.Write("[Get]ServiceShm key not match!|%s|%s", Pkg->service().servicename().c_str(), servicedata.servicename().c_str());
					result = 102;
				}
				else
				{
					for(int i = 0; i < servicedata.attrs_size(); i++)
					{
						bool Add = true;
						if(Pkg->service().days_size() > 0)
						{
							Add = false;
							string key(Pkg->service().servicename()); 
							key.append(1u,0x3).append(servicedata.attrs(i).name());
							string skey(tce::TC_MD5::md5bin(key));	//md5 + dayvalue
							skey.append(4u,0);	//增加空位
							for(int j = 0; j < Pkg->service().days_size(); j++)
							{
								uint32_t day = Pkg->service().days(j);
								skey.replace(skey.end()-4, skey.end(), (char*)&day, sizeof(uint32_t));
								if(DataShm.HasKey(skey))
								{
									Add = true;
									break;
								}
							}
						}
						if(Add)	
							service->add_attrnames(servicedata.attrs(i).name());
					}
					for(int i = 0; i < servicedata.ips_size(); i++)
					{
						service->add_ips(tce::InetNtoA(servicedata.ips(i).ip()));
					}
					
					result = 0;
				}
			}
		}
		else
		{
			result = 0;
		}
	}

	resp.set_result(result);
	msg_log.Write("[Get]%s|%d|%lu", str.str().c_str(), result, tc.value());
	return SendRespPkg(stSession, resp);
}

int CGetProcCenter::ProcServiceAttr(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	int result = 0;
	ostringstream str;
	msec::monitor::RespMonitor resp;

	str << "ServiceAttr|";
	for(int i =0; i < Pkg->serviceattr().attrnames_size(); i++)
	{
		str << Pkg->serviceattr().attrnames(i) << "|";
		for(int j = 0; j < Pkg->serviceattr().days_size(); j++)
		{
			if( i == 0)
				str << Pkg->serviceattr().days(j) << "|";
			char valuedata[1440*4];		//属性按天数据
			if(!GetValueData(Pkg->serviceattr().servicename(), Pkg->serviceattr().attrnames(i), "", Pkg->serviceattr().days(j), valuedata))
			{					
				msec::monitor::RespServiceAttr* serviceattr = resp.mutable_serviceattr();
				msec::monitor::AttrData* attrdata = serviceattr->add_data();
				attrdata->set_attrname(Pkg->serviceattr().attrnames(i));
				attrdata->set_day(Pkg->serviceattr().days(j));
				for(int k = 0; k < 1440; k++)	
				{
					attrdata->add_values(*(uint32_t*)&valuedata[k*4]);
				}
			}
		}
	}
	
	resp.set_result(result);
	msg_log.Write("[Get]%s|%d|%lu", str.str().c_str(), result, tc.value());
	return SendRespPkg(stSession, resp);
}

int CGetProcCenter::ProcAttrIP(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	int result = 0;
	ostringstream str;
	msec::monitor::RespMonitor resp;

	str << "AttrIP|";
	for(int i = 0; i < Pkg->attrip().ips_size(); i++)
	{
		str << Pkg->attrip().ips(i) << "|";
		for(int j =0; j < Pkg->attrip().days_size(); j++)
		{
			if(i == 0)
				str << Pkg->attrip().days(j) << "|";
			char valuedata[1440*4];		//属性按天数据
			if(!GetValueData(Pkg->attrip().servicename(), Pkg->attrip().attrname(), Pkg->attrip().ips(i), Pkg->attrip().days(j), valuedata))
			{					
				msec::monitor::RespAttrIP* attrip = resp.mutable_attrip();
				msec::monitor::AttrIPData* attripdata = attrip->add_data();
				attripdata->set_ip(Pkg->attrip().ips(i));
				attripdata->set_day(Pkg->attrip().days(j));
				for(int k = 0; k < 1440; k++)	//直接到头
				{
					attripdata->add_values(*(uint32_t*)&valuedata[k*4]);
				}
			}
		}
	}
	
	resp.set_result(result);
	msg_log.Write("[Get]%s|%d|%lu", str.str().c_str(), result, tc.value());
	return SendRespPkg(stSession, resp);
}

int CGetProcCenter::ProcIP(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	int result = 100;
	ostringstream str;
	msec::monitor::RespMonitor resp;
	char data[128 * 1024];
	int data_len = sizeof(data);

	str << "IP|";
	
	uint32_t ip = tce::InetAtoN(Pkg->ip().ip());
	if(ip == 0)
	{
		err_log.Write("[Get]GetNode IP error!|%s", Pkg->ip().ip().c_str());
		result = 101;
	}
	else
	{
		str << Pkg->ip().ip() << "|";
		string key( (char*)&ip, sizeof(uint32_t));			
		int ret = ServiceShm.GetNode(key, &data[0], data_len);
		if( ret != 0)
		{
			err_log.Write("[Get]GetNode ServiceShm failed!|%s|%d|%s", Pkg->ip().ip().c_str(), ret, ServiceShm.GetErrorMsg());
			result = 102;
		}
		else
		{
			
			msec::monitor::data::IPData ipdata; 	//IP - 属性数据
			if(data_len > 0)
			{
				if(!ipdata.ParseFromArray(data, data_len))
				{
					err_log.Write("[Get]Parse from ServiceShm failed!|%s", Pkg->ip().ip().c_str());
					result = 103;
				}
				else
				{
					result = 0;
					msec::monitor::RespIP* respip = resp.mutable_ip();
					for(int i = 0; i < ipdata.services_size(); i++)
					{
						msec::monitor::IPData* data = respip->add_data();
						data->set_servicename(ipdata.services(i).servicename());
						for(int j = 0; j < ipdata.services(i).attrs_size(); j++)
						{
							bool Add = true;
							if(Pkg->ip().days_size() > 0)
							{
								Add = false;
								string key(ipdata.services(i).servicename()); 
								key.append(1u,0x3).append(ipdata.services(i).attrs(j).name());
								
								string skey(tce::TC_MD5::md5bin(key));	//md5 + dayvalue
								skey.append((char*)&ip, sizeof(uint32_t));
								skey.append(4u,0);	//增加空位
								for(int k 	= 0; k < Pkg->ip().days_size(); k++)
								{
									uint32_t day = Pkg->ip().days(k);
									skey.replace(skey.end()-4, skey.end(), (char*)&day, sizeof(uint32_t));
									if(DataShm.HasKey(skey))
									{
										Add = true;
										break;
									}
								}
							}
							if(Add)
								data->add_attrnames(ipdata.services(i).attrs(j).name());
						}
					}						
				}
			}
			else
			{
				result = 0;
			}
		}
	}

	resp.set_result(result);
	msg_log.Write("[Get]%s|%d|%lu", str.str().c_str(), result, tc.value());
	return SendRespPkg(stSession, resp);
}

int CGetProcCenter::ProcIPAttr(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	int result = 0;
	ostringstream str;
	msec::monitor::RespMonitor resp;

	result = 0;
	str << "IPAttr|";
	for(int i = 0; i < Pkg->ipattr().attrs_size(); i++)
	{
		str << Pkg->ipattr().attrs(i).servicename() << "|";
		for(int j = 0; j < Pkg->ipattr().attrs(i).attrnames_size(); j++)
		{
			str << Pkg->ipattr().attrs(i).attrnames(j) << "|";
			for(int k =0; k < Pkg->ipattr().days_size(); k++)
			{
				if(j == 0)
					str << Pkg->ipattr().days(k) << "|";						
				char valuedata[1440*4]; 	//属性按天数据
				if(!GetValueData(Pkg->ipattr().attrs(i).servicename(), Pkg->ipattr().attrs(i).attrnames(j), Pkg->ipattr().ip(), Pkg->ipattr().days(k), valuedata))
				{					
					msec::monitor::RespIPAttr* ipattr = resp.mutable_ipattr();
					msec::monitor::IPAttrData* ipattrdata = ipattr->add_data();
					ipattrdata->set_servicename(Pkg->ipattr().attrs(i).servicename());
					ipattrdata->set_attrname(Pkg->ipattr().attrs(i).attrnames(j));
					ipattrdata->set_day(Pkg->ipattr().days(k));
					for(int l = 0; l < 1440; l++)	//直接到头
					{
						ipattrdata->add_values(*(uint32_t*)&valuedata[l*4]);
					}
				}
			}
		}
	}

	resp.set_result(result);
	msg_log.Write("[Get]%s|%d|%lu", str.str().c_str(), result, tc.value());
	return SendRespPkg(stSession, resp);
}


int CGetProcCenter::ProcSetAlarmAttr(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	msec::monitor::RespMonitor resp;
	int result = 100;
	size_t affected_rows = 0;
	if(stConfig.bAlarm)
	{
		ostringstream sql;
		thrData * d = wbl::thread_pool::get_thread_data<thrData>();	// 取得线程的私有数据

		//检查是否能插入数据
		if(CJudgeProcCenter::GetInstance().CheckSetAlarmAttr(Pkg->setalarmattr().servicename(), Pkg->setalarmattr().attrname()))
		{
			sql << "insert into attr(servicename, attrname, max_num, min_num, diff_num, diffp_num) values ('"
				<< d->mysql_alarm.escape_string(Pkg->setalarmattr().servicename()) << "','"
				<< d->mysql_alarm.escape_string(Pkg->setalarmattr().attrname()) << "',"
				<< Pkg->setalarmattr().max() << ","
				<< Pkg->setalarmattr().min() << ","
				<< Pkg->setalarmattr().diff() << ","
				<< Pkg->setalarmattr().diff_percent() << ") on duplicate key update ";

			bool need_comma = false;
			if(Pkg->setalarmattr().has_max())
			{
				need_comma ? (sql << ", "):(need_comma = true);
				sql << "max_num = " << Pkg->setalarmattr().max();
			}
			if(Pkg->setalarmattr().has_min())
			{
				need_comma ? (sql << ", "):(need_comma = true);
				sql << "min_num = " << Pkg->setalarmattr().min();
			}
			if(Pkg->setalarmattr().has_diff())
			{
				need_comma ? (sql << ", "):(need_comma = true);
				sql << "diff_num = " << Pkg->setalarmattr().diff();
			}
			if(Pkg->setalarmattr().has_diff_percent())
			{
				need_comma ? (sql << ", "):(need_comma = true);
				sql << "diffp_num = " << Pkg->setalarmattr().diff_percent();
			}

			try{
				const monitor::MySqlData& data = d->mysql_alarm.query(sql.str());
				affected_rows = data.affected_rows();	//注意，落到on duplicate key update的场景下affected_rows为2，见mysql manual
				result = 0;	//执行成功则result置0
			}
			catch(std::runtime_error& e)
			{
				result = 101;
				err_log.Write("[Get]SetAlarmAttr|ERR|SQL|%s", e.what());
			}
		}
		else
		{
			result = 1;	//该服务无法插入数据项
		}
	}
	else
	{
		result = 102;
	}
	resp.set_result(result);
	msg_log.Write("[Get]SetAlarmAttr|%s|%s|%d|%d|%d|%d|%lu|%d|%lu", Pkg->setalarmattr().servicename().c_str(), Pkg->setalarmattr().attrname().c_str(),
		Pkg->setalarmattr().has_max()?Pkg->setalarmattr().max():-1, Pkg->setalarmattr().has_min()?Pkg->setalarmattr().min():-1,
		Pkg->setalarmattr().has_diff()?Pkg->setalarmattr().diff():-1, Pkg->setalarmattr().has_diff_percent()?Pkg->setalarmattr().diff_percent():-1,
		affected_rows, result, tc.value());
	return SendRespPkg(stSession, resp);
}

int CGetProcCenter::ProcDelAlarmAttr(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	int result = 0;
	msec::monitor::RespMonitor resp;
	size_t affected_rows = 0;
	if(stConfig.bAlarm)
	{
		ostringstream sql;
		thrData * d = wbl::thread_pool::get_thread_data<thrData>();	// 取得线程的私有数据
		
		sql << "delete from attr where servicename = '"
			<< d->mysql_alarm.escape_string(Pkg->delalarmattr().servicename()) << "' and attrname = '"
			<< d->mysql_alarm.escape_string(Pkg->delalarmattr().attrname()) << "'";

		try{
			const monitor::MySqlData& data = d->mysql_alarm.query(sql.str());
			affected_rows = data.affected_rows();
			msec::monitor::RespDelAlarmAttr* respdelalarmattr = resp.mutable_delalarmattr();
			respdelalarmattr->set_deleted_rows(affected_rows);
		}
		catch(std::runtime_error& e)
		{
			result = 101;
			err_log.Write("[Get]DelAlarmAttr|ERR|SQL|%s", e.what());
		}
	}
	else
	{
		result = 102;
	}
	resp.set_result(result);
	msg_log.Write("[Get]DelAlarmAttr|%s|%s|%lu|%d|%lu", Pkg->delalarmattr().servicename().c_str(), Pkg->delalarmattr().attrname().c_str(), affected_rows, result, tc.value());
	return CGetProcCenter::GetInstance().SendRespPkg(stSession, resp);
}

int CGetProcCenter::ProcNewestAlarm(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	msec::monitor::RespMonitor resp;
	int result = 0;
	int size = 0;
	if(stConfig.bAlarm)
	{
		ostringstream date;
		ostringstream sql;
		thrData * d = wbl::thread_pool::get_thread_data<thrData>();	// 取得线程的私有数据
		
		msec::monitor::RespNewestAlarm* respnewalarm = resp.mutable_newalarm();
		date << Pkg->newalarm().day()/10000 << "-" << (Pkg->newalarm().day()%10000)/100 << "-" << Pkg->newalarm().day()%100;

		sql << "select servicename, attrname, alarm_type, unix_timestamp(min(alarmtime)) as first_time, unix_timestamp(max(alarmtime)) as last_time, count(*) as count, "
			<< "substring_index(group_concat(cast(alarm_type as char), ';', cast(current_num as char), ';', cast(alarm_num as char) order by alarmtime desc), ',', 1) as last_value from alarm where alarmtime between '"
			<< date.str() << " 00:00:00' and '" << date.str() << " 23:59:59' ";
		
		if(Pkg->newalarm().has_servicename() && !Pkg->newalarm().servicename().empty())
			sql << "and servicename='" << Pkg->newalarm().servicename() <<  "' ";
		sql << "group by servicename, attrname, alarm_type order by last_time desc";
		
		try{
			const monitor::MySqlData& data = d->mysql_alarm.query(sql.str());
			for(size_t i = 0; i < data.num_rows(); ++i){
				const monitor::MySqlRowData& r = data[i];
				msec::monitor::Alarm* alarm = respnewalarm->add_alarms();
				alarm->set_servicename(r["servicename"]);
				alarm->set_attrname(r["attrname"]);
				alarm->set_first_alarm_time(wbl::s2u(r["first_time"]));
				alarm->set_last_alarm_time(wbl::s2u(r["last_time"]));
				alarm->set_alarm_times(wbl::s2u(r["count"]));
				
				std::stringstream ss(r["last_value"]);
				std::string value;
				std::getline(ss, value, ';');
				alarm->set_type(msec::monitor::AlarmType(wbl::s2u(value)));
				std::getline(ss, value, ';');
				alarm->set_last_value(wbl::s2u(value));
				std::getline(ss, value, ';');
				alarm->set_last_attr_value(wbl::s2u(value));
			}
		}
		catch(std::runtime_error& e)
		{
			result = 101;
			err_log.Write("[Get]NewestAlarm|ERR|SQL|%s", e.what());
		}
		size = respnewalarm->alarms_size();
	}
	else
	{
		result = 102;
	}
	resp.set_result(result);
	msg_log.Write("[Get]NewestAlarm|%s|%u|%d|%d|%lu", Pkg->newalarm().servicename().c_str(), Pkg->newalarm().day(), size, result, tc.value());
	return CGetProcCenter::GetInstance().SendRespPkg(stSession, resp);
}

int CGetProcCenter::ProcDelAlarm(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	msec::monitor::RespMonitor resp;
	int result = 0;
	size_t affected_rows = 0;
	if(stConfig.bAlarm)
	{
		ostringstream sql;
		thrData * d = wbl::thread_pool::get_thread_data<thrData>();	// 取得线程的私有数据
		
		ostringstream date;
		date << Pkg->delalarm().day()/10000 << "-" << (Pkg->delalarm().day()%10000)/100 << "-" << Pkg->delalarm().day()%100;
		
		sql << "delete from alarm where servicename = '"
			<< d->mysql_alarm.escape_string(Pkg->delalarm().servicename()) << "' and attrname = '"
			<< d->mysql_alarm.escape_string(Pkg->delalarm().attrname()) << "' and alarm_type = "
			<< Pkg->delalarm().type() << " and alarmtime between '"
			<< date.str() << " 00:00:00' and '" << date.str() << " 23:59:59' and unix_timestamp(alarmtime) <= " << Pkg->delalarm().time();
			
		try{
			const monitor::MySqlData& data = d->mysql_alarm.query(sql.str());
			affected_rows = data.affected_rows();
			msec::monitor::RespDelAlarm* respdelalarm = resp.mutable_delalarm();
			respdelalarm->set_deleted_rows(affected_rows);
		}
		catch(std::runtime_error& e)
		{
			result = 101;
			err_log.Write("[Get]DelAlarm|ERR|SQL|%s", e.what());
		}
	}
	else
	{
		result = 102;
	}
	resp.set_result(result);
	msg_log.Write("[Get]DelAlarm|%s|%s|%u%u%u|%lu|%d|%lu", Pkg->delalarm().servicename().c_str(), Pkg->delalarm().attrname().c_str(),
		Pkg->delalarm().type(), Pkg->delalarm().day(), Pkg->delalarm().time(), affected_rows, result, tc.value());
	return CGetProcCenter::GetInstance().SendRespPkg(stSession, resp);
}

int CGetProcCenter::ProcTreeList(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg)
{
	tce::CTimeCost tc;
	int result = 0;
	msec::monitor::RespMonitor resp;

	msec::monitor::RespTreeList* resptreelist = resp.mutable_treelist();
 	tce::ReadLocker rl(lock_treelist);
	set<string> keys;
	ServiceShm.GetKeys(keys);
	for(set<string>::iterator it = keys.begin(); it != keys.end(); it++)	
	{
		msec::monitor::TreeListInfo* info = resptreelist->add_infos();
		info->set_servicename(*it);
	}	

	resp.set_result(result);
	msg_log.Write("[Get]TreeList|%d|%d|%lu", keys.size(), result, tc.value());

	return SendRespPkg(stSession, resp);
}


int CGetProcCenter::OnRead(tce::SSession& stSession, const unsigned char* pszData, const size_t nSize)
{
	// 解析接收的包
	std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg(new msec::monitor::ReqMonitor());
	
	if ( !Pkg->ParseFromArray(pszData,nSize) )
	{
		err_log.Write("[Get]Parse from string failed! src info: (%s:%u)", stSession.GetIPByStr().c_str(), stSession.GetPort());
		return -1;
	}

	if(Pkg->has_service())
	{
		//请求类型1 :
		//输入: servicename
		//输出: attrnames, ips
		//return ProcService(stSession, Pkg);
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcService, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_serviceattr())
	{
		//请求类型2:
		//输入: svcname, attrnames, days
		//输出: 多天的多属性上报值
		//return ProcServiceAttr(stSession, Pkg);
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcServiceAttr, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_attrip())
	{
		//请求类型3:
		//输入: ips, svcname, attrname, days
		//输出：单个属性的多天多ip的上报值
		//return ProcAttrIP(stSession, Pkg);
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcAttrIP, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_ip())
	{
		//请求类型4：
		//输入: ip
		//输出: svcnames, attrnames
		//return ProcIP(stSession, Pkg);
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcIP, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_ipattr())
	{
		//请求类型5:
		//输入: svcnames, attrs, ip, days
		//输出: 单个IP的多天多属性数据
		//return ProcIPAttr(stSession, Pkg);
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcIPAttr, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_setalarmattr())
	{
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcSetAlarmAttr, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_delalarmattr())
	{
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcDelAlarmAttr, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_newalarm())
	{
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcNewestAlarm, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_delalarm())
	{
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcDelAlarm, wbl::ref(stSession), Pkg);
		return 0;
	}
	else if(Pkg->has_treelist())
	{
		m_pool.execute_memfun(CGetProcCenter::GetInstance(), &CGetProcCenter::ProcTreeList, wbl::ref(stSession), Pkg);
		return 0;
	}
	return 0;
}

void CGetProcCenter::OnClose(tce::SSession& stSession)
{
	m_tCurTime = time(NULL);
	msg_log.Write("[Get]OnClose! ip: %s, port: %u", stSession.GetIP().c_str(), stSession.GetPort());
}

void CGetProcCenter::OnConnect(tce::SSession& stSession, const bool bConnectOk)
{
	m_tCurTime = time(NULL);
	if ( bConnectOk )
	{
		msg_log.Write("[Get]OnConnect! ip: %s, port: %u", stSession.GetIP().c_str(), stSession.GetPort());
	}
}

void CGetProcCenter::OnError(tce::SSession& stSession, const int32_t iErrCode, const char* pszErrMsg)
{
	m_tCurTime = time(NULL);
	err_log.Write("[Get]OnError! err_code: %u, ip: %s, err_msg: %s", iErrCode, stSession.GetIP().c_str(), 
			pszErrMsg?pszErrMsg:"");
}
