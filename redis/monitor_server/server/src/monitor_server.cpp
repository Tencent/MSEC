
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
 *       Filename:  monitor_server.cpp
 *
 *    Description:  主函数
 *
 *        Version:  1.0
 *       Revision:  none
 *       Compiler:  g++
 *
 *        Company:  Tencent
 *
 * =====================================================================================
 */
#include "tce.h"
#include "set_proc_center.h"
#include "get_proc_center.h"
#include "judge_proc_center.h"
#include "dump_proc_center.h"
#include "proc_def.h"

#include <iostream>
#include "shm_mgr.h"

using namespace std;

tce::CFileLog msg_log;
tce::CFileLog err_log;

SConfig stConfig;
CShmMgr ServiceShm;
CShmMgr DataShm;


bool InitConfig(SConfig& stConfig) 
{
	// 加载配置信息 
	tce::CConfig oCfg;
	if ( !oCfg.LoadConfig(stConfig.sCfgFile) ) 
	{
		cout << "load config file error:" << oCfg.GetErrMsg() <<endl;
		return false;
	}

	// Daemon进程
	oCfg.GetValue("comm", "Daemon", stConfig.bDaemon, true);

	// Core信息
	oCfg.GetValue("sys", "UsingCore", stConfig.bUsingCore, false);
	oCfg.GetValue("sys", "MaxOpenFile", stConfig.dwMaxOpenFile, MAX_OPEN_FILE);
	oCfg.GetValue("sys", "MaxCoreFile", stConfig.dwMaxCoreFile, MAX_CORE_FILE);
	oCfg.GetValue("sys", "ThreadPoolSize", stConfig.dwThreadPoolSize, 32);

	// 信号量信息
	oCfg.GetValue("sem", "key", stConfig.dwSemKey, 0x30083);

	// 共享内存
	oCfg.GetValue("shm", "memory_percent", stConfig.wMemoryPercentage, 0);
	oCfg.GetValue("shm", "memory_left", stConfig.dwMemoryLeft, 0);
	oCfg.GetValue("shm", "service_key", stConfig.dwServiceShmKey, 0x47000);
	oCfg.GetValue("shm", "data_key", stConfig.dwDataShmKey, 0x47001);
	unsigned long memory = tce::getSystemMemory();

	if(stConfig.wMemoryPercentage == 0)
	{		if(memory < 1900000000)
		{
			cout << "Current machine memory is smaller than 2GB." << endl;
			return false;
		}
		else if(memory < 3900000000UL)
		{
			//小内存
			stConfig.dwMemoryLeft = 1024 * 1024 * 1024;
			stConfig.wMemoryPercentage = 70;			
		}
		else
		{
			//大内存
			stConfig.dwMemoryLeft = (uint32_t)2048 * 1024 * 1024;
			stConfig.wMemoryPercentage = 90;
		}
	}
	else
	{
		if(stConfig.dwMemoryLeft > 0)
		{
			stConfig.dwMemoryLeft*=1024*1024;	//Bytes
			if(memory < stConfig.dwMemoryLeft )
			{
				cout << "Current machine memory is smaller than " << stConfig.dwMemoryLeft << " Bytes" << endl;
				return false;
			}
		}
		if(stConfig.wMemoryPercentage > 90)
			stConfig.wMemoryPercentage = 90;
	}
	
	unsigned long use_memory = (memory - stConfig.dwMemoryLeft)/100 * stConfig.wMemoryPercentage;
	stConfig.ddwServiceShmSize = use_memory / 11000000 * 1000000;
	stConfig.ddwDataShmSize = stConfig.ddwServiceShmSize * 10;
	
	if(stConfig.ddwServiceShmSize < 30000000 || stConfig.ddwDataShmSize < 300000000 )
	{
		cout << "The shared memory size is too small, please reconfigure." << endl;
		return false;
		
	}
	oCfg.GetValue("shm", "dump_path", stConfig.sDumpPath, "../data/dump");

	printf("[shm info]:\n");
	printf("\tservice: key: %#x, size: %lu\n\tdata: key: %#x, size: %lu\n", stConfig.dwServiceShmKey, stConfig.ddwServiceShmSize, stConfig.dwDataShmKey, stConfig.ddwDataShmSize);

	// 定时器设置 
	oCfg.GetValue("timer", "to_check_signal", stConfig.dwCheckSignalInterval, 500);

	printf("[Timer Info]: \n");
	printf("\t Signal Timer Interval: %u ms\n", stConfig.dwCheckSignalInterval);

	// log信息
	oCfg.GetValue("log", "LogPath", stConfig.sLogPath, "/msec/monitor/log/");
	oCfg.GetValue("log", "LogFileSize", stConfig.dwLogFileSize, 5*1024*1024u);
	oCfg.GetValue("log", "LogFileCount", stConfig.dwLogFileCount, 5u);
	oCfg.GetValue("log", "ShowCmd", stConfig.bShowCmd, false);

	// 初始化日志
	stConfig.dwLogFileSize = max(stConfig.dwLogFileSize, 1*1024*1024u);
	stConfig.dwLogFileCount = max(stConfig.dwLogFileCount, 1u);

	err_log.Init(stConfig.sLogPath+"/"+"err", stConfig.dwLogFileSize, 
			stConfig.dwLogFileCount, false);
	msg_log.Init(stConfig.sLogPath+"/"+"msg", stConfig.dwLogFileSize, 
			stConfig.dwLogFileCount, false);

	printf("[Log]: \n");
	printf("\tLog Dir: %s\n", stConfig.sLogPath.c_str());
	printf("\tLog file size:%u, file count:%u\n", stConfig.dwLogFileSize, stConfig.dwLogFileCount);

	// 设置limit
	printf("[System]: \n");
	if( !tce::set_file_limit(stConfig.dwMaxOpenFile) ) 
	{
		printf("[err]Open files limit|%u|%s\n", stConfig.dwMaxOpenFile, strerror(errno));
		err_log.Write("[err]Open files limit|%u|%s", stConfig.dwMaxOpenFile, strerror(errno));
		return false;
	}
	else
	{
		printf("\tOpen files: %u\n", stConfig.dwMaxOpenFile);
	}

	if ( stConfig.bUsingCore ) 
	{
		if ( !tce::set_core_limit(stConfig.dwMaxCoreFile) ) 
		{
			printf("[err]Core file size limit|%u|%s\n", stConfig.dwMaxCoreFile, strerror(errno));
			err_log.Write("[err]Core file size limit|%u|%s", stConfig.dwMaxCoreFile, strerror(errno));
		}
		else
		{
			printf("\tCore file size: %u\n", stConfig.dwMaxCoreFile);
		}
	}

	//localip
	oCfg.GetValue("sys", "LocalIP", stConfig.sLocalIP, "127.0.0.1");
	if(stConfig.sLocalIP != "127.0.0.1")
	{
		set<string> ips;
		if(tce::GetAllLocalIP(ips) || ips.find(stConfig.sLocalIP) == ips.end())
		{
			printf("[err]LocalIP setting error:%s\n", stConfig.sLocalIP.c_str());
			err_log.Write("[err]LocalIP setting error:%s", stConfig.sLocalIP.c_str());
			return false;
		}
	}	

	// tcp服务器信息
	oCfg.GetValue("tcp_svr", "ip", stConfig.sTcpIp, "127.0.0.1");
	oCfg.GetValue("tcp_svr", "set_port", stConfig.wSetTcpPort, 30083);
	oCfg.GetValue("tcp_svr", "get_port", stConfig.wGetTcpPort, 30084);

	oCfg.GetValue("tcp_svr", "max_client", stConfig.dwTcpMaxClient, 1000);
	oCfg.GetValue("tcp_svr", "over_time", stConfig.dwTcpOverTime, 300);
	oCfg.GetValue("tcp_svr", "in_buf_size", stConfig.dwTcpInBufSize, 50*1024*1024);
	oCfg.GetValue("tcp_svr", "out_buf_size", stConfig.dwTcpOutBufSize, 50*1024*1024);

	printf("[TCP Server]\n");
	printf("\tIP: %s, Port: %u|%u.\n", stConfig.sTcpIp.c_str(), stConfig.wSetTcpPort, stConfig.wGetTcpPort);
	printf("\tMaxClient: %u.\n", stConfig.dwTcpMaxClient);
	printf("\tTiemout: %u s.\n", stConfig.dwTcpOverTime);
	printf("\tBuffer size: In=%u|Out=%u.\n", stConfig.dwTcpInBufSize, stConfig.dwTcpOutBufSize);


	//告警DB	
	oCfg.GetValue("alarm", "UsingAlarm", stConfig.bAlarm, false);
	oCfg.GetValue("alarm", "DBHost", stConfig.sAlarmDBHost, "127.0.0.1");
	oCfg.GetValue("alarm", "DBPort", stConfig.wAlarmDBPort, 3306);
	oCfg.GetValue("alarm", "DBUser", stConfig.sAlarmDBUser, "root");
	oCfg.GetValue("alarm", "DBPassword", stConfig.sAlarmDBPassword, "password");
	oCfg.GetValue("alarm", "DBName", stConfig.sAlarmDBName, "alarm_db");
	oCfg.GetValue("alarm", "MaxAlarmNumPerService", stConfig.wMaxAlarmNumPerService, 5);		//每个业务默认最多设置5个告警

	//Redis DB	
	oCfg.GetValue("redis_console", "DBHost", stConfig.sRedisDBHost, "127.0.0.1");
	oCfg.GetValue("redis_console", "DBPort", stConfig.wRedisDBPort, 3306);
	oCfg.GetValue("redis_console", "DBUser", stConfig.sRedisDBUser, "root");
	oCfg.GetValue("redis_console", "DBPassword", stConfig.sRedisDBPassword, "password");
	oCfg.GetValue("redis_console", "DBName", stConfig.sRedisDBName, "redis_db");
	oCfg.GetValue("redis_console", "CheckInterval", stConfig.dwCheckRedisDBInterval, 5);	//5s检测1次

	// DB配置信息 
	printf("[Database]: \n");
	printf("\tAlarmDB Info: %s@%s:%u#%s\n", stConfig.sAlarmDBUser.c_str(), stConfig.sAlarmDBHost.c_str(), stConfig.wAlarmDBPort, stConfig.sAlarmDBName.c_str());
	printf("\tRedisDB Info: %s@%s:%u#%s\n", stConfig.sRedisDBUser.c_str(), stConfig.sRedisDBHost.c_str(), stConfig.wRedisDBPort, stConfig.sRedisDBName.c_str());

	if (stConfig.bDaemon) 
	{
		cout << "Server starts as a daemon." << endl;
		tce::init_daemon();
	}

	return true;
}


bool InitShm(SConfig& stConfig)
{
	MhtInitParam param_service;
	bool created = false;
	bool load_dump = false;
	memset((char*)&param_service, 0, sizeof(MhtInitParam));
	param_service.cMaxKeyLen = 16;
	param_service.ddwShmKey = stConfig.dwServiceShmKey;
	param_service.ddwBufferSize = stConfig.ddwServiceShmSize;
	param_service.dwExpiryRatio = 95;
	int ret = ServiceShm.Init(param_service, true, created);
	if(ret != 0)
	{
		printf("error creating service storage|%d|%s\n", ret, ServiceShm.GetErrorMsg());
		return false;
	}
	msg_log << "ServiceKeys|" << ServiceShm.KeysSize() << endl;
	if (created)	load_dump = true;
	
	MhtInitParam param_data;
	memset((char*)&param_data, 0, sizeof(MhtInitParam));
	param_data.cMaxKeyLen = 24;
	param_data.ddwShmKey = stConfig.dwDataShmKey;
	param_data.ddwBufferSize = stConfig.ddwDataShmSize;
	param_data.dwIndexRatio = 100;
	param_data.ddwBlockSize = value_fixed_len+8;	//一个Block需要额外8个字节
	param_data.dwExpiryRatio = 95;
	ret = DataShm.Init(param_data, false, created);
	if(ret != 0)
	{
		printf("error creating data storage|%d|%s\n", ret, DataShm.GetErrorMsg());
		return false;
	}
	if (created)	load_dump = true;

	CDumpProcCenter::GetInstance().Init(stConfig.sDumpPath);
	if (load_dump)
	{
		msg_log << "Need to load dump data." << endl;
		CDumpProcCenter::GetInstance().LoadData();
	}
	return true;
}

int OnSetRead(tce::SSession& stSession, const unsigned char* pszData, const size_t nSize){
	return CSetProcCenter::GetInstance().OnRead(stSession, pszData, nSize);
}

void OnSetConnect(tce::SSession& stSession, const bool bConnectOk){
	CSetProcCenter::GetInstance().OnConnect(stSession, bConnectOk);
}

void OnSetClose(tce::SSession& stSession){
	CSetProcCenter::GetInstance().OnClose(stSession);
}

int OnGetRead(tce::SSession& stSession, const unsigned char* pszData, const size_t nSize){
	return CGetProcCenter::GetInstance().OnRead(stSession, pszData, nSize);
}

void OnGetConnect(tce::SSession& stSession, const bool bConnectOk){
	CGetProcCenter::GetInstance().OnConnect(stSession, bConnectOk);
}

void OnGetClose(tce::SSession& stSession){
	CGetProcCenter::GetInstance().OnClose(stSession);
}

void OnError(tce::SSession& stSession, const int32_t iErrCode, const char* pszErrMsg){
	CSetProcCenter::GetInstance().OnError(stSession, iErrCode, pszErrMsg);
}

void OnTimer(const int iId){
	CSetProcCenter::GetInstance().OnTimer(iId);
}

bool IsUniqueProc(int iSemKey) 
{
	tce::CSemaphore oSem;
	if ( !oSem.Create(iSemKey) ) 
	{
		cout << "Create semaphore error! SemKey: " << iSemKey << ", Msg: " << oSem.GetErrMsg() << endl;
		return false;
	}

	if ( !oSem.Lock(false) ) 
	{
		cout << "semaphore P operation failed! Msg: " << oSem.GetErrMsg() << endl;
		return false;
	}

	return true;
}

static void BeginToDump(int signo)
{
    if (signo == SIGUSR1)
    {
        CDumpProcCenter::GetInstance().SetDumpOn();
    }
}

int main(int argc, char* argv[])
{
	// 文件名
	stConfig.sProgramName = argv[0];

	// 配置文件信息
	if ( argc > 1 )
	{
		stConfig.sCfgFile = argv[1];
	}
	else
	{
		stConfig.sCfgFile = "../conf/monitor_server.ini";
	}

	// 初始化配置信息
	if ( !InitConfig(stConfig) ) 
	{
		return 0;
	}
	
	if ( !InitShm(stConfig) )
	{
		return 0;
	}

	//mysql初始化
	if (mysql_library_init(0, NULL, NULL)) {
        cout << "mysql_library_init failed." << endl;
		err_log.Write("mysql_library_init failed.");
		return 0;
    }

	bool bRet = false;

	if(stConfig.bAlarm)
	{
		// 初始化CJudgeProcCenter
		bRet = CJudgeProcCenter::GetInstance().Init();
		if ( !bRet )
		{
			cout << "CJudgeProcCenter init failed." << endl;
			err_log << "CJudgeProcCenter initialize failed!" << endl;
			return 1;
		}
	}

	// 进程互斥锁, 保证同时最多只有一个进程在执行
	if ( !IsUniqueProc(stConfig.dwSemKey) ) 
	{
		cout << "Only one copy of process is permitted to run. Current process is quitting." << endl;
		err_log.Write("Only one copy of process is permitted to run. Current process is quitting.");
		return 0;
	}

	// 设置回调函数
	tce::CCommMgr::GetInstance().SetTimerCallbackFunc(&OnTimer);

	// 创建tcp服务器
	int iSetTcpID = tce::CCommMgr::GetInstance().CreateSvr(tce::CCommMgr::CT_TCP_SVR, stConfig.sTcpIp, stConfig.wSetTcpPort,
			stConfig.dwTcpInBufSize, stConfig.dwTcpOutBufSize);
	if ( iSetTcpID < 0 ) 
	{
		cout << "create set tcp svr error:" << tce::CCommMgr::GetInstance().GetErrMsg() << endl;
		err_log << "create set tcp svr error:" << tce::CCommMgr::GetInstance().GetErrMsg() << endl;
		return 1;
	}

	// socket内存管理方式
	tce::CCommMgr::GetInstance().SetSvrSockManageType(iSetTcpID, tce::CCommMgr::SMT_ARRAY);
	// 设置TCP分包方式
	tce::CCommMgr::GetInstance().SetSvrDgramType(iSetTcpID, tce::CCommMgr::TDT_LONGLEN_INCLUDE_ITSELF);
	// 设置网络事件回调函数 
	tce::CCommMgr::GetInstance().SetSvrCallbackFunc(iSetTcpID, &OnSetRead, &OnSetClose, &OnSetConnect, &OnError);
	//设置SOCKET参数
	tce::CCommMgr::GetInstance().SetSvrClientOpt(iSetTcpID, stConfig.dwTcpMaxClient, stConfig.dwTcpOverTime, 2*1024*1024, 64*1024*1024, 64*1024*1024, 64*1024);

	CSetProcCenter::GetInstance().SetTcpID(iSetTcpID);
	// 初始化CSetProcCenter
	bRet = CSetProcCenter::GetInstance().Init();
	if ( !bRet )
	{
		cout << "CSetProcCenter init failed." << endl;
		err_log << "CSetProcCenter initialize failed!" << endl;
		return 1;
	}

	// 创建tcp服务器
	int iGetTcpID = tce::CCommMgr::GetInstance().CreateSvr(tce::CCommMgr::CT_TCP_SVR, stConfig.sTcpIp, stConfig.wGetTcpPort,
			stConfig.dwTcpInBufSize, stConfig.dwTcpOutBufSize);
	if ( iGetTcpID < 0 ) 
	{
		cout << "create get tcp svr error:" << tce::CCommMgr::GetInstance().GetErrMsg() << endl;
		err_log << "create get tcp svr error:" << tce::CCommMgr::GetInstance().GetErrMsg() << endl;
		return 1;
	}

	// socket内存管理方式
	tce::CCommMgr::GetInstance().SetSvrSockManageType(iGetTcpID, tce::CCommMgr::SMT_ARRAY);
	// 设置TCP分包方式
	tce::CCommMgr::GetInstance().SetSvrDgramType(iGetTcpID, tce::CCommMgr::TDT_LONGLEN_INCLUDE_ITSELF);
	// 设置网络事件回调函数 
	tce::CCommMgr::GetInstance().SetSvrCallbackFunc(iGetTcpID, &OnGetRead, &OnGetClose, &OnGetConnect, &OnError);
	//设置SOCKET参数
	tce::CCommMgr::GetInstance().SetSvrClientOpt(iGetTcpID, stConfig.dwTcpMaxClient, stConfig.dwTcpOverTime, 2*1024*1024, 64*1024*1024, 64*1024*1024, 64*1024);

	CGetProcCenter::GetInstance().SetTcpID(iGetTcpID);
	// 初始化CGetProcCenter
	bRet = CGetProcCenter::GetInstance().Init();
	if ( !bRet )
	{
		cout << "CGetProcCenter init failed." << endl;
		err_log << "CGetProcCenter initialize failed!" << endl;
		return 1;
	}

	cout << "Svr Initialized OK!" << endl;

	// 运行所有服务（以上的服务）
	if ( !tce::CCommMgr::GetInstance().RunAllSvr() ) 
	{
		cout << "run svr error:" << tce::CCommMgr::GetInstance().GetErrMsg() << endl;
		err_log << "run svr error:" << tce::CCommMgr::GetInstance().GetErrMsg() << endl;
		return 0;
	}

	CDumpProcCenter::GetInstance().Start();
	signal(SIGUSR1, BeginToDump);
	
	cout << "Program start running in background..." << endl;
	msg_log << "Program start running in background...\n" << endl;

	// 启动主线程循环
	tce::CCommMgr::GetInstance().Start(); 

	CDumpProcCenter::GetInstance().Stop();

	return 0;
}
