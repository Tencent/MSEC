
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


#include "signal_handler.h"
#include <string.h>

const int CSigHandler::iMaxSignalCnt = 256;
CSigHandler* CSigHandler::m_pInstance = NULL;

CSigHandler::CSigHandler(void):m_iSigCnt(0)	
{ 
	m_vecSigInfo.resize(iMaxSignalCnt); 
	for (size_t i = 0; i < m_vecSigInfo.size(); i++)
	{
		memset(&m_vecSigInfo[i], 0, sizeof(SIG_INFO));
	}
}

CSigHandler::~CSigHandler() {}

bool CSigHandler::SetSigHander(int iSignal, HANDLESIGNAL_FUNC pfHandleSignal)
{
	if ( !IsValidSignal(iSignal) )
	{
		return false;
	}

	if (NULL == pfHandleSignal)
	{
		return false;
	}

	m_vecSigInfo[iSignal].iStatus = SS_INACTIVE; /* 初始是未触发的状态 */
	m_vecSigInfo[iSignal].pfHandleSignal = pfHandleSignal;

	this->signal(iSignal, OnSignalSet);

	return true;
}

bool CSigHandler::Check(void *pUserInfo)
{
	for (int i = 0; i < iMaxSignalCnt && m_iSigCnt > 0; i++)
	{
		if ( SS_ACTIVE == m_vecSigInfo[i].iStatus )
		{
			m_iSigCnt--;                        /* 恢复状态 */
			m_vecSigInfo[i].iStatus = SS_INACTIVE;

			(*m_vecSigInfo[i].pfHandleSignal)(i, pUserInfo); /* 调用回调函数 */
		}
	}

	return true;
}
