
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


#ifndef __TCE_FIFO_BUFFER_BY_HEAD_H__
#define __TCE_FIFO_BUFFER_BY_HEAD_H__

#include "fifo_buffer.h"

namespace tce{

template<template T>
class CFIFOBufferByHead :
	private CNonCopyAble
{
	typedef CFIFOBufferByHead this_type;
	typedef T HEAD;
public:
	CFIFOBufferByHead()
		:m_iId(0)
	{}

	~CFIFOBufferByHead(){}

	bool Init(const int iId, const int iBufLen){
		bool bOk = false;
		bOk = m_oFIFOBuffer.Init(iBufLen);
		if (bOk)
		{
			m_iId = iId;
			bOk = true;
		}
		return bOk;
	}
	int GetId() const	{	return m_iId;	}

	const char* GetErrMsg() const {	return m_oFIFOBuffer.GetErrMsg();	}

	CFIFOBuffer::RETURN_TYPE Read(HEAD& stHead, unsigned char* pszBuf, int& iBufLen){	
		string sReadBuf;
		CFIFOBuffer::RETURN_TYPE nRe = m_oFIFOBuffer.Read(sReadBuf);
		if ( nRe == CFIFOBuffer::BUF_OK )
		{
			if ( sReadBuf.size() >= sizeof(HEAD) )
			{
				memcpy(&stHead, sReadBuf.data(), sizeof(HEAD));
				memcpy(pszBuf, sReadBuf.data()+sizeof(HEAD), sReadBuf.size()-sizeof(HEAD));
			}
			else
			{
				nRe = CFIFOBuffer::BUF_ERR;
			}
		}
		return nRe;
	}
	CFIFOBuffer::RETURN_TYPE Read(HEAD& stHead, string& sReadBuf)
	{
		CFIFOBuffer::RETURN_TYPE nRe = m_oFIFOBuffer.Read(sReadBuf);
		if ( nRe == CFIFOBuffer::BUF_OK )
		{
			if ( sReadBuf.size() >= sizeof(HEAD) )
			{
				memcpy(&stHead, sReadBuf.data(), sizeof(HEAD));
				sReadBuf.erase(0, sizeof(HEAD));
			}
			else
			{
				nRe = CFIFOBuffer::BUF_ERR;
			}
		}
		return nRe;
	}

	CFIFOBuffer::RETURN_TYPE Write(const HEAD& stHead, const unsigned char* pszBuf, const int iBufLen)
	{
		return this->Write(stHead, reinterpret_cast<const char*>(pszBuf), iBufLen);
	}
	CFIFOBuffer::RETURN_TYPE Write(const HEAD& stHead, const char* pszBuf, const int iBufLen)	{		
		string sWriteBuf;
		sWriteBuf.assign(static_cast<char*>(&stHead), sizeof(stHead));
		sWriteBuf.append(pszBuf, iBufLen);
		return m_oFIFOBuffer.Write(sWriteBuf.data(), sWriteBuf.size());	
	}
	CFIFOBuffer::RETURN_TYPE Write(const HEAD& stHead, const string& sBuf)	{	
		return this->Write(stHead, sBuf.data(), sBuf.size());
	}

private:

	CFIFOBuffer m_oFIFOBuffer;
	int m_iId;						//buffer ID

};

};

#endif

