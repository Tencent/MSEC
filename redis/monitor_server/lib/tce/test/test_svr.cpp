
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


#include <iostream>
#include "tce.h"

using namespace std;
using namespace tce;

tce::CFileLog g_oFileLog;

#ifdef WIN32
#define SVR_IP "127.0.0.1"
#else
#define SVR_IP "127.0.0.1"
#endif


int g_iHttpCommID = 0;
int g_iTcpCommID = 0;
int g_iUdpCommID = 0;

//网络数据事件触发OnRead函数
int OnRead(tce::SSession& stSession, const unsigned char* pszData, const size_t iSize){


	if ( g_iHttpCommID == stSession.GetCommID() )
	{
		//http 解析
		tce::CHttpParser oParser;
		if ( oParser.Decode((char*)pszData, iSize) )
		{
			cout << "URI:" << oParser.GetURI() << endl;

			if ( strcmp(oParser.GetURI(), "/cgi-bin/test") == 0 )
			{
				//response
				tce::CHttpResponse oResp;
				oResp.Begin();
				oResp << "test123456";
				oResp.End();
				tce::CCommMgr::GetInstance().Write(stSession, oResp.GetData(), oResp.GetDataLen());
			} 
			else if ( strcmp(oParser.GetURI(), "/cgi-bin/test1") == 0 )
			{
				//response
				tce::CHttpResponse oResp;
				oResp.Begin();
				oResp << "haha";
				oResp.End();
				tce::CCommMgr::GetInstance().Write(stSession, oResp.GetData(), oResp.GetDataLen());
			}
			else
			{
				//response
				tce::CHttpResponse oResp;
				oResp.Begin();
				oResp << "no support request:" << oParser.GetURI();
				oResp.End();
				tce::CCommMgr::GetInstance().Write(stSession, oResp.GetData(), oResp.GetDataLen());
			}
		}
		else
		{
			cout << "error:" << oParser.GetErrMsg() << endl;
		}
	} 
	else if ( g_iTcpCommID == stSession.GetCommID() )
	{
		cout << "tcp:" << (char*)pszData << endl;
		tce::CCommMgr::GetInstance().Write(stSession, pszData, iSize);
		//do tcp data
	} 
	else
	{
		//do udp data
		cout << "udp:" << (char*)pszData << endl;
		tce::CCommMgr::GetInstance().Write(stSession, pszData, iSize);
	}


	return 0;
}

//tcp链接关闭触发OnClose函数
void OnClose(tce::SSession& stSession){
//	cout << "OnClose" << endl;
}

//主动链接成功或失败触发该函数，bConnectOk: 建立链接成功标志
void OnConnect(tce::SSession& stSession, const bool bConnectOk){
//	cout << "OnConnect:" << bConnectOk << endl;
}

//定时器触发该函数//iId参数为	tce::CCommMgr::GetInstance().SetTimer(1, 100, true); 第一个的参数
void OnTimer(const int iId){

}

//底层错误触发
void OnError(tce::SSession& stSession, const int iErrNo, const char* pszMsg){	cout << "OnError" << endl;}



int main(void)
{
	g_oFileLog.Init("./test",10*1024);

	//建立HTTP SVR
	g_iHttpCommID = tce::CCommMgr::GetInstance().CreateSvr(CCommMgr::CT_HTTP_SVR, SVR_IP, 30005, 1000000,1000000);
	tce::CCommMgr::GetInstance().SetSvrCallbackFunc(g_iHttpCommID, &OnRead, &OnClose, &OnConnect, &OnError);

	//建立UDP SVR
	g_iUdpCommID = tce::CCommMgr::GetInstance().CreateSvr(CCommMgr::CT_UDP_SVR, SVR_IP, 30003, 1000000,1000000);
	tce::CCommMgr::GetInstance().SetSvrCallbackFunc(g_iUdpCommID, &OnRead, &OnClose, &OnConnect, &OnError);

	//建立TCP SVR
	g_iTcpCommID = tce::CCommMgr::GetInstance().CreateSvr(CCommMgr::CT_TCP_SVR, SVR_IP, 30003, 5000000,5000000);
	tce::CCommMgr::GetInstance().SetSvrDgramType(g_iTcpCommID,CCommMgr::TDT_H2LONGT3);
	tce::CCommMgr::GetInstance().SetSvrCallbackFunc(g_iTcpCommID, &OnRead, &OnClose, &OnConnect, &OnError);


	//运行所有服务（以上的服务）
	tce::CCommMgr::GetInstance().RunAllSvr();

	//设置定时器回调函数
	tce::CCommMgr::GetInstance().SetTimerCallbackFunc(&OnTimer);

	//设置一个定时器
	tce::CCommMgr::GetInstance().SetTimer(1, 100, true);


	//启动通信模块
	tce::CCommMgr::GetInstance().Start();
	return 0;
}

