
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
 *       Filename:  judge_proc_center.h
 *
 *    Description:  告警逻辑处理中心
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  g++
 *
 *        Company:  Tencent
 *
 * =====================================================================================
 */
#ifndef __JUDGE_PROC_CENTER_H__
#define __JUDGE_PROC_CENTER_H__

#include "tce.h"
#include "tce_singleton.h"
#include "tce_thread.h"
#include "mysql_wrapper.h"
#include "tce_lock.h"

#include <set>
#include <tr1/memory>

#include "monitor.pb.h"

class AttrKey {
public:
	string ServiceName;
	string AttrName;
	uint32_t Max;
	uint32_t Min;
	uint32_t Diff;
	uint32_t DiffP;
	bool operator < ( const AttrKey &rhs ) const
	{
		return ( ServiceName == rhs.ServiceName ? (AttrName < rhs.AttrName) : ( ServiceName < rhs.ServiceName ) );
	}
};

class JudgeKey {
public:
	uint32_t Date;
	uint32_t Minute;
	uint32_t MinTime;
	AttrKey Key;
	bool operator < ( const JudgeKey &rhs ) const
	{
		return ( MinTime == rhs.MinTime ? ( Key < rhs.Key ) : ( MinTime < rhs.MinTime ) );
	}
};


class CJudgeProcCenter
	: tce::CNonCopyAble
{
public:
	DECLARE_SINGLETON_CLASS(CJudgeProcCenter);

public:
	bool Init(void);
	void CheckAlarm();
	
	uint32_t getMinTime(uint32_t date, uint32_t min);
	uint32_t getDay(time_t Time);
	
	void AddJudge(const string& ServiceName, const string& AttrName, uint32_t Date, uint32_t Minute, uint32_t Value);	
	static int32_t Judge(void* pParam);
	void JudgeMax(const JudgeKey& Key, uint32_t Value, uint32_t Max);
	void JudgeMin(const JudgeKey& Key, uint32_t Value, uint32_t Min);
	void JudgeDiff(const JudgeKey& Key, uint32_t Value, uint32_t Diff, uint32_t DiffP);
	void JudgeDiffCalc(const JudgeKey& Key, uint32_t Value, string& preData, uint32_t Diff, uint32_t DiffP);
	void Stop();

	void ProcServiceAlarmAttr(msec::monitor::RespService* service, const string& ServicNeame);
	bool CheckSetAlarmAttr(const string& ServiceName, const string& AttrName);

private:
	CJudgeProcCenter(void);
	
	monitor::CMySql mysql_alarm;
	std::tr1::shared_ptr<map<string, set<AttrKey> > > alarm_attrs;
	mutable tce::CMutex lock_judge;
	map<JudgeKey, uint32_t> judge_map;
	mutable tce::ReadWriteLocker lock_month;
	map<uint32_t, uint32_t> month_map;	//key: month like 201604, value: actual minute

	map<JudgeKey, string> mem_judge_map;	//data in mem

	typedef int32_t (* THREADFUNC)(void *);
	typedef tce::CThread<THREADFUNC> THREAD;	
	THREAD judge_thread;
	bool run_judge;
};

#endif
