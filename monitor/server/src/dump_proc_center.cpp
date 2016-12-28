
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


#include "dump_proc_center.h"

#include "log_def.h"
#include "proc_def.h"
#include "signal_handler.h"
#include "monitor.pb.h"
#include "data.pb.h"
#include "set_proc_center.h"
#include <fstream>

CDumpProcCenter::CDumpProcCenter()
	: m_oTraverseThread(&TraverseThread),
	m_bTraverseProcessRun(false),
	m_bDumpOn(false)
{}

CDumpProcCenter::~CDumpProcCenter() 
{}

bool CDumpProcCenter::Init(const string& sDumpPath)
{
	m_sDumpPath = sDumpPath;
	return true;
}

//加载Dump文件中的数据到共享内存
int CDumpProcCenter::LoadData()
{
	string sLoadPath = m_sDumpPath;
	if (access(sLoadPath.c_str(), R_OK) != 0)
	{
		err_log << "No dump data file exist:  " << sLoadPath << endl;
		return 0;
	}

	ifstream fin(sLoadPath.c_str(), ios::in);
	if (!fin)
	{
		err_log << "Open file failed: " << sLoadPath << endl;
		return -1;
	}

	string line;
	string ip;
	string day_str;
	int day = 0;
	string svcname;
	string attrname;
	int value_size = 1440;
	uint32_t value_array[1440];

	msec::monitor::ReqReport req;
	int ret = 0;
	int count = 0;
	while (getline(fin, line))
	{
		stringstream sstream;
		
		ip = ReadEscapedString(line);
		day_str = ReadEscapedString(line);
		svcname = ReadEscapedString(line);
		attrname = ReadEscapedString(line);
		
		if (ip.empty() || day_str.empty() 
		   || svcname.empty() || attrname.empty() || line.empty())
			continue;

		day = strtoul(day_str.c_str(), NULL, 10);
		sstream << line;

		int i = 0;
		int got = 0;

		req.Clear();
		msec::monitor::Attr* attr = req.add_attrs();
		for (i=0; i<value_size; ++i)
		{
			sstream >> value_array[i];
			attr->add_values(value_array[i]);
			if (sstream.fail())
				break;
			++got;
		}

		if (got != value_size)
			continue;

		string sTime = tce::ToStr(day) + "000000";
		time_t tBeginTime = tce::GetDateTime(sTime);
		attr->set_servicename(svcname);
		attr->set_attrname(attrname);
		attr->set_begin_time(tBeginTime);
		attr->set_end_time(tBeginTime + value_size * 60 - 1);

		//数据加载到内存
		ret = CSetProcCenter::GetInstance().ProcessReq(req, ip, false);
		if (ret != 0)
		{
			err_log << "Load data into memory failed. " << req.DebugString() << endl;
		}
		else 
		{
			++count;
		}
	}
	fin.close();

	if (count > 0)
	{
		msg_log.Write("[Dump]Load OK|%u", count);
	}
	else
	{
		err_log.Write("[Dump]No data loaded!");
	}
	return 0;
}

bool CDumpProcCenter::Start()
{
	m_oTraverseThread.Start(this);
	return true;
}

int32_t CDumpProcCenter::TraverseThread(void* pParam)
{
	CDumpProcCenter* poThis = (CDumpProcCenter*)pParam;
	if ( NULL != poThis )
	{
	       poThis->m_bTraverseProcessRun = true;
		while (poThis->m_bTraverseProcessRun)
		{
			if (poThis->m_bDumpOn)
			{
				msg_log.Write("[Dump]Dump Begins.");
				poThis->TraverseProcess();
				poThis->m_bDumpOn = false;
			}
			tce::xsleep(1000);
		}
	}

	return 0;
}

int32_t CDumpProcCenter::TraverseProcess()
{
	int ret = 0;
	uint32_t numeric_ip;
	uint32_t day;
	int dump_cnt = 0;

	string sDumpPathNew = m_sDumpPath + ".new";
	ofstream fout(sDumpPathNew.c_str(), ios::out);
	if (!fout)
	{
		err_log << "Open file failed: " << m_sDumpPath << endl;
		return -1;
	}

	MhtIterator it;
	MhtData* data = new MhtData();
	//遍历DataShm, 数据dump为文件
	for (it = DataShm.ht.Begin(); it != DataShm.ht.End(); it = DataShm.ht.Next(it))
	{
		if ( !m_bTraverseProcessRun )
		{
			break;
		}
		
		ret = DataShm.GetData(it, *data);
		if (ret != 0)
		{
			err_log.Write("DataShm traverse failed. ret: %d", ret);
			break;
		}

		//KeyFormat: MD5(svc + 0x03 + attr) + [ dwIP ] + dwDay
		//MD5 length: 16 byte
		//dwIP: 	   4 byte
		//dwDay:  4byte
		if (data->klen != 24)  //没有IP的过滤掉
			continue;

		memcpy(&numeric_ip, data->key + 16, sizeof(numeric_ip));
		memcpy(&day, data->key + 20, sizeof(day));

		if(data->dlen == value_fixed_len)
		{		
			string svcname = (char*)&data->data[0];
			string attrname = (char*)&data->data[128];

			EscapeForBlank(svcname);
			EscapeForBlank(attrname);
			fout << tce::InetNtoA(numeric_ip) << " " 
				<< day << " "
				<< svcname << " " 
				<< attrname;

			char* ptr = (char*)&data->data[256];
			for (int i=0; i< 1440; ++i)
			{
				fout << " " << *(uint32_t*)(ptr+i*4);
			}
			fout << endl;

			++dump_cnt;
			if (dump_cnt % 10000 == 0)   tce::xsleep(500);
		}
	}
	delete data;

	fout.close();
	rename(sDumpPathNew.c_str(), m_sDumpPath.c_str()); //Rename dump file
	return 0;
}

void CDumpProcCenter::EscapeForBlank(string& str)
{
	if (str.empty()) return;

	string ret;
	for (size_t i=0; i<str.length(); ++i)
	{
		if (str[i] == ' ') 
		{
			ret += "\\ ";
		}
		else if (str[i] == '\\') 
		{
			ret += "\\\\";
		}
		else
		{
			ret += str[i];
		}
	}
	str = ret;
}

void CDumpProcCenter::UnescapeForBlank(string& str)
{
	if (str.empty()) return;

	string ret;
	for (size_t i=0; i<str.length(); ++i)
	{
		if (str[i] == '\\') 
		{
			if (i + 1 < str.length() && str[i+1] == ' ')
			{
				++i;
				ret += ' ';
			}
			else if (i + 1 < str.length() && str[i+1] == '\\')
			{
				++i;
				ret += '\\';
			}
			else
			{
				ret += str[i];
			}
		}
		else
		{
			ret += str[i];
		}
	}
	str = ret;
}
	
string CDumpProcCenter::ReadEscapedString(string& str)
{
	size_t i = 0;
	string ret;
	
	if (str.empty())  return ret;
	
	while (str[i] != '\0')
	{
		if (str[i] == ' ' && (i==0 || str[i-1]!='\\'))
			break;
		
		ret += str[i++];
	}

	if (i+1 < str.length())
		str = str.substr(i + 1);
    	else 
		str = "";

       UnescapeForBlank(ret);
	return ret;
}
