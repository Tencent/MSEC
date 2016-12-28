
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


#include "judge_proc_center.h"
#include "get_proc_center.h"
#include "log_def.h"
#include "proc_def.h"
#include "signal_handler.h"
#include "monitor.pb.h"
#include "data.pb.h"
#include <sstream>
#include "wbl_comm.h"
 
CJudgeProcCenter::CJudgeProcCenter(void)
	:alarm_attrs(new map<string, set<AttrKey> >),
	judge_thread(&Judge),
	run_judge(false)
{}

void CJudgeProcCenter::Stop()
{
	run_judge=false;
	judge_thread.Stop();
}

bool CJudgeProcCenter::Init(void)
{
	try{
		//告警DB初始化
		mysql_alarm.Init(stConfig.sAlarmDBHost, stConfig.sAlarmDBUser, stConfig.sAlarmDBPassword, stConfig.wAlarmDBPort);
		mysql_alarm.use(stConfig.sAlarmDBName);
		CheckAlarm();	//获取第一份数据
	}
	catch(exception& e)
	{
		cout << "AlarmDB init failed. err: " << e.what() <<  endl;
		err_log << "AlarmDB init failed. err: " << e.what() << endl;
		return false;
	}
	judge_thread.Start(this);
	return true;
}

void CJudgeProcCenter::CheckAlarm()
{
	//开始获取数据
	std::tr1::shared_ptr<map<string, set<AttrKey> > > attrs(new map<string, set<AttrKey> >);
	static uint32_t updatetime = 0;
	bool flag = false;
	try{
		const monitor::MySqlData& data = mysql_alarm.query("select servicename, attrname, max_num, min_num, diff_num, diffp_num, unix_timestamp(updatetime) as time from attr");
		for(size_t i = 0; i < data.num_rows(); ++i){
			const monitor::MySqlRowData& r = data[i];
			AttrKey key;
			key.ServiceName = r["servicename"];
			key.AttrName = r["attrname"];
			key.Max = wbl::s2u(r["max_num"]);
			key.Min = wbl::s2u(r["min_num"]);
			key.Diff = wbl::s2u(r["diff_num"]);
			key.DiffP = wbl::s2u(r["diffp_num"]);
			uint32_t time = wbl::s2u(r["time"]);
			if(updatetime < time)
			{
				updatetime = time;
				flag = true;
			}
			if(key.Max != 0 || key.Min != 0 || key.Diff != 0 || key.DiffP != 0)	//如果4个都设为0，则认为此项监控项待删除
			{
				(*attrs.get())[key.ServiceName].insert(key);
			}
			}
		alarm_attrs = attrs;
		if(flag)
			msg_log.Write("[Judge]Check OK|%u|%u", attrs->size(), updatetime);
	}
	catch(std::runtime_error& e)
	{
		err_log.Write("[Judge]Check ERR: %s", e.what());
	}
}

uint32_t CJudgeProcCenter::getMinTime(uint32_t date, uint32_t min)
{
	if(date <= 20100000 || min >= 1440)
		return 0;
	
	{
		tce::ReadLocker rl(lock_month);
		map<uint32_t, uint32_t>::iterator it = month_map.find(date/100);
		if(	it != month_map.end())
		{
			return it->second + (date%100-1)*1440 + min;
		}
	}
	
	time_t rawtime;
	struct tm timeinfo;

	/* get current timeinfo and modify it */
	time ( &rawtime );
	localtime_r( &rawtime, &timeinfo );
	timeinfo.tm_year = date/10000 - 1900;
	timeinfo.tm_mon = (date%10000)/100 - 1;
	timeinfo.tm_mday = 1;	//only get first day
	timeinfo.tm_hour = 0;
	timeinfo.tm_min = 0;
	timeinfo.tm_sec = 0;
	
	uint32_t mintime = mktime(&timeinfo)/60;

	{
		tce::WriteLocker wl(lock_month);
		month_map[date/100] = mintime;
	}
	return mintime + (date%100-1)*1440 + min;
	
}

uint32_t CJudgeProcCenter::getDay(time_t Time)
{
	struct tm timeinfo;
	localtime_r( &Time, &timeinfo );
	return (timeinfo.tm_year+1900) * 10000 + (timeinfo.tm_mon + 1) * 100 + timeinfo.tm_mday;
}

void CJudgeProcCenter::ProcServiceAlarmAttr(msec::monitor::RespService* service, const string& ServiceName)
{
	map<string, set<AttrKey> >::iterator it = alarm_attrs->find(ServiceName);
	if(it != alarm_attrs->end())
	{
		for(set<AttrKey>::iterator itattr = it->second.begin(); itattr != it->second.end(); itattr++)
		{
			msec::monitor::AlarmAttr* alarmattr = service->add_alarmattrs();
			alarmattr->set_attrname(itattr->AttrName);
			alarmattr->set_max(itattr->Max);
			alarmattr->set_min(itattr->Min);
			alarmattr->set_diff(itattr->Diff);
			alarmattr->set_diff_percent(itattr->DiffP);
		}
	}
}

bool CJudgeProcCenter::CheckSetAlarmAttr(const string& ServiceName, const string& AttrName)
{
	map<string, set<AttrKey> >::iterator it = alarm_attrs->find(ServiceName);
	if(it != alarm_attrs->end())
	{
		AttrKey key;
		key.ServiceName = ServiceName;
		key.AttrName = AttrName;
		if(it->second.find(key) == it->second.end())
		{
			if( it->second.size() >= stConfig.wMaxAlarmNumPerService)
			{
				return false;
			}
		}
	}
	return true;
}

//date使用yyyymmdd格式
void CJudgeProcCenter::AddJudge(const string& ServiceName, const string& AttrName, uint32_t Date, uint32_t Minute, uint32_t Value)	
{

	JudgeKey judgekey;
	judgekey.Key.ServiceName = ServiceName;
	judgekey.Key.AttrName = AttrName;
	map<string, set<AttrKey> >::iterator it = alarm_attrs->find(judgekey.Key.ServiceName);
	if(it != alarm_attrs->end())
	{
		if(it->second.find(judgekey.Key) != it->second.end())
		{
			judgekey.Date = Date;
			judgekey.Minute = Minute;
			judgekey.MinTime = getMinTime(Date, Minute);
			if(judgekey.MinTime != 0)
			{
				tce::CAutoLock lock(lock_judge);
				judge_map[judgekey] = Value;
			}
			msg_log.Write("[Judge]Add|%s|%s|%u|%u|%u", ServiceName.c_str(), AttrName.c_str(), Date, Minute, Value);
		}
	}
}

//单线程，计算无需线程保护
int32_t CJudgeProcCenter::Judge(void* pParam)
{
	CJudgeProcCenter* poThis = (CJudgeProcCenter*)pParam;
	if ( NULL != poThis )
	{
		poThis->run_judge = true;
		time_t Now = time(NULL);
		static uint32_t CurMin = Now/60;
		while(poThis->run_judge && stConfig.bAlarm)
		{
			Now = time(NULL);
			uint32_t CurSec = Now % 60;
			map<JudgeKey, uint32_t> JudgeSlice;
			if(CurMin != Now/60 && CurSec > 35)	//每分钟只做一次，35为magic number
			{
				CurMin = Now/60;
				poThis->CheckAlarm();
				tce::CAutoLock lock(poThis->lock_judge);		//取数据和计算分离
				map<JudgeKey, uint32_t>::iterator it;
				for(it = poThis->judge_map.begin(); it != poThis->judge_map.end(); it++)
				{
					if(it->first.MinTime >= CurMin)
						break;
				}
				if(it != poThis->judge_map.begin())
				{
					//insert, erase [first, last)
					JudgeSlice.insert(poThis->judge_map.begin(),it);
					poThis->judge_map.erase(poThis->judge_map.begin(),it);
				}
			}

			//计算
			for(map<JudgeKey, uint32_t>::iterator it = JudgeSlice.begin(); it != JudgeSlice.end(); it++)
			{
				const JudgeKey& key = it->first;
				map<string, set<AttrKey> >::iterator itattrmap = poThis->alarm_attrs->find(key.Key.ServiceName);
				if(itattrmap != poThis->alarm_attrs->end())
				{
					set<AttrKey>::iterator itattr = itattrmap->second.find(key.Key);
					if(itattr != itattrmap->second.end())
					{
						msg_log.Write("[Judge]Calc|%s|%s|%u|%u|%u", key.Key.ServiceName.c_str(), key.Key.AttrName.c_str(), CurMin, key.MinTime, it->second);
						poThis->JudgeMax(key, it->second, itattr->Max);
						poThis->JudgeMin(key, it->second, itattr->Min);
						poThis->JudgeDiff(key, it->second, itattr->Diff, itattr->DiffP);
					}
				}
			}
			if(CurSec == 0)
				msg_log.Write("[Judge]Size|%u|%u|%u", poThis->judge_map.size(), poThis->month_map.size(), poThis->mem_judge_map.size());
		    sleep(1);
		}
	}
	return 0;
}

void CJudgeProcCenter::JudgeMax(const JudgeKey& Key, uint32_t Value, uint32_t Max)
{
	if(Max != 0 && Value >= Max)
	{
		std::ostringstream str;
		str << "insert into alarm(id, servicename, attrname, createtime, alarmtime, alarm_type, current_num, alarm_num) values (0, '"
			<< mysql_alarm.escape_string(Key.Key.ServiceName) << "', '"
			<< mysql_alarm.escape_string(Key.Key.AttrName) 
 			<< "', NOW(), FROM_UNIXTIME(" << Key.MinTime*60 << "), 1, "	//type =1, max
 			<< Value << ", "
 			<< Max << ")";
		try{
			const monitor::MySqlData& data = mysql_alarm.query(str.str());
			if(data.auto_id() != 0)
			{
				msg_log.Write("[Judge]Alarm|Max|%s|%s|%u|%u|%u", Key.Key.ServiceName.c_str(), Key.Key.AttrName.c_str(), Key.MinTime, Value, Max);
			}
		}
		catch(std::runtime_error& e)
		{
			err_log.Write("[Judge]Alarm|Max|ERR|%s|%s|%u|%u|%u|%s", Key.Key.ServiceName.c_str(), Key.Key.AttrName.c_str(), Key.MinTime, Value, Max, e.what());
		}
	}
}

void CJudgeProcCenter::JudgeMin(const JudgeKey& Key, uint32_t Value, uint32_t Min)
{
	if(Min != 0 && Value <= Min)
	{
		std::ostringstream str;
		str << "insert into alarm(id, servicename, attrname, createtime, alarmtime, alarm_type, current_num, alarm_num) values (0, '"
			<< mysql_alarm.escape_string(Key.Key.ServiceName) << "', '"
			<< mysql_alarm.escape_string(Key.Key.AttrName) 
 			<< "', NOW(), FROM_UNIXTIME(" << Key.MinTime*60 << "), 2, "	//type = 2, min
 			<< Value << ", "
 			<< Min << ")";
		try{
			const monitor::MySqlData& data = mysql_alarm.query(str.str());
			if(data.auto_id() != 0)
			{
				msg_log.Write("[Judge]Alarm|Min|%s|%s|%u|%u|%u", Key.Key.ServiceName.c_str(), Key.Key.AttrName.c_str(), Key.MinTime, Value, Min);
			}
		}
		catch(std::runtime_error& e)
		{
			err_log.Write("[Judge]Alarm|Min|ERR|%s|%s|%u|%u|%u|%s", Key.Key.ServiceName.c_str(), Key.Key.AttrName.c_str(), Key.MinTime, Value, Min, e.what());
		}
	}
}

void CJudgeProcCenter::JudgeDiff(const JudgeKey& Key, uint32_t Value, uint32_t Diff, uint32_t DiffP)
{
	if(Diff != 0)
	{
		JudgeKey preKey = Key;
		preKey.MinTime -= 1440+Key.Minute;	//上一天的数据

		map<JudgeKey, string>::iterator it = mem_judge_map.find(preKey);
		if(it == mem_judge_map.end())
		{
			static char data[1440*4];
			int ret = CGetProcCenter::GetInstance().GetValueData(preKey.Key.ServiceName, preKey.Key.AttrName, "", getDay(preKey.MinTime * 60), data);
			msg_log.Write("[Judge]MemAdd|%u|%u|%u", Key.Date, preKey.MinTime, getDay(preKey.MinTime * 60));
			if( ret != 0)
				mem_judge_map[preKey] = "";	//为空
			else
			{
				string s(data, 1440*4);
				mem_judge_map[preKey] =s;
				JudgeDiffCalc(Key, Value, s, Diff, DiffP);
			}
		}
		else
		{
			JudgeDiffCalc(Key, Value, it->second, Diff, DiffP);
		}
	}
}

void CJudgeProcCenter::JudgeDiffCalc(const JudgeKey& Key, uint32_t Value, string& preData, uint32_t Diff, uint32_t DiffP)
{
	if(preData.size() == 0)
		return;		//不比较昨天数据为空的监控
	uint32_t preValue = *(uint32_t*)(preData.data() + Key.Minute* 4);
	if(Diff != 0)
	{
		uint32_t Wave = abs((int)Value - (int)preValue);
		if( Wave >= Diff )
		{
			std::ostringstream str;
			str << "insert into alarm(id, servicename, attrname, createtime, alarmtime, alarm_type, current_num, alarm_num) values (0, '"
				<< mysql_alarm.escape_string(Key.Key.ServiceName) << "', '"
				<< mysql_alarm.escape_string(Key.Key.AttrName) 
					<< "', NOW(), FROM_UNIXTIME(" << Key.MinTime*60 << "), 3, "	//type = 3, diff
					<< Value << ", "
					<< Diff << ")";
			try{
				const monitor::MySqlData& data = mysql_alarm.query(str.str());
				if(data.auto_id() != 0)
				{
					msg_log.Write("[Judge]Alarm|Diff|%s|%s|%u|%u|%u|%u|%u|%u", Key.Key.ServiceName.c_str(), Key.Key.AttrName.c_str(), Key.Date, Key.Minute, Value, preValue, Wave, Diff);
				}
			}
			catch(std::runtime_error& e)
			{
				err_log.Write("[Judge]Alarm|Diff|ERR|%s|%s|%u|%u|%u|%u|%u|%u|%s", Key.Key.ServiceName.c_str(), Key.Key.AttrName.c_str(), Key.Date, Key.Minute, Value, preValue, Wave, Diff, e.what());
			}
		}
	}
	if(DiffP != 0)
	{
		uint32_t Wave = abs((int)Value - (int)preValue) * 100 / ( preValue > Value ? Value : preValue );
		if( Wave >= DiffP )
		{
			std::ostringstream str;
			str << "insert into alarm(id, servicename, attrname, createtime, alarmtime, alarm_type, current_num, alarm_num) values (0, '"
				<< mysql_alarm.escape_string(Key.Key.ServiceName) << "', '"
				<< mysql_alarm.escape_string(Key.Key.AttrName) 
					<< "', NOW(), FROM_UNIXTIME(" << Key.MinTime*60 << "), 4, "	//type = 4, diffp
					<< Value << ", "
					<< DiffP << ")";
			try{
				const monitor::MySqlData& data = mysql_alarm.query(str.str());
				if(data.auto_id() != 0)
				{
					msg_log.Write("[Judge]Alarm|DiffP|%s|%s|%u|%u|%u|%u|%u|%u", Key.Key.ServiceName.c_str(), Key.Key.AttrName.c_str(), Key.Date, Key.Minute, Value, preValue, Wave, DiffP);
				}
			}
			catch(std::runtime_error& e)
			{
				err_log.Write("[Judge]Alarm|DiffP|ERR|%s|%s|%u|%u|%u|%u|%u|%u|%s", Key.Key.ServiceName.c_str(), Key.Key.AttrName.c_str(), Key.Date, Key.Minute, Value, preValue, DiffP, e.what());
			}
		}
	}
	
}
