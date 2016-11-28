
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

//网络数据事件触发OnRead函数
int OnRead(tce::SSession& stSession, const unsigned char* pszData, const size_t iSize){

	//http 解析
	tce::CHttpParser oParser;
	if ( oParser.Decode((char*)pszData, iSize) )
	{
		cout << "URI:" << oParser.GetURI() << endl;

		if ( strcmp(oParser.GetURI(), "/cgi-bin/test") == 0 )
		{
			if ( oParser.GetIfModifiedSinceTime() + 60 < time(NULL) )	//oParser.GetIfModifiedSinceTime() 获取IE页面修改时间
			{
				tce::CHttpResponse oResp;
				oResp.Begin();
				oResp.SetStatusCode(304);
				oResp.End();
				tce::CCommMgr::GetInstance().Write(stSession, oResp.GetData(), oResp.GetDataLen());
				return 0;
			}

			//response
			tce::CHttpResponse oResp;
			oResp.Begin();
			oResp.SetLastModified(time(NULL));			//设置页面修改时间
			oResp.SetCacheControl("max-age=10");	//设置页面不访问服务器时间（单位秒）
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
void OnError(tce::SSession& stSession, const int iErrNo, const char* pszErrMsg){	cout << "OnError" << endl;}



int main(void)
{
	g_oFileLog.Init("./test",10*1024);

	//建立HTTP SVR
	int iCommID = tce::CCommMgr::GetInstance().CreateSvr(CCommMgr::CT_HTTP_SVR, SVR_IP, 30005, 1000000,1000000);
	tce::CCommMgr::GetInstance().SetSvrCallbackFunc(iCommID, &OnRead, &OnClose, &OnConnect, &OnError);


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

