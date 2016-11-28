
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


#ifndef __LOG_DEF_H__
#define __LOG_DEF_H__
#include "file_log.h"

extern int g_USE_MSG_LOG_FLAG;
extern int g_USE_ERR_LOG_FLAG;
extern tce::CFileLog msg_log;
extern tce::CFileLog err_log;
extern tce::CFileLog trace_log;

#include <iostream>
using namespace std;


#ifdef USE_MSG_LOG
#define MSG_LOG(format, ...) MsgLog(__FILE__, __LINE__, format, ## __VA_ARGS__)
#else
#define MSG_LOG(format, ...)
#endif

#ifdef USE_ERR_LOG
#define ERR_LOG(format, ...) ErrLog(__FILE__, __LINE__, format, ## __VA_ARGS__)
#else
#define ERR_LOG(format, ...)
#endif

#define STAT_LOG(format, ...) StatLog(__FILE__, __LINE__, format, ## __VA_ARGS__)


#ifdef USE_MSG_LOG
//#define MSG_LOG( exp )			MsgLog(__FILE__,__LINE__,exp)
#define MSG_LOG0( exp )			MsgLog(__FILE__,__LINE__,exp)
#define MSG_LOG1( exp, param1 )	MsgLog(__FILE__,__LINE__, exp, param1 )
#define MSG_LOG2( exp, param1, param2 ) MsgLog(__FILE__,__LINE__, exp, param1, param2 )
#define MSG_LOG3( exp, param1, param2, param3 ) MsgLog(__FILE__,__LINE__, exp, param1, param2,param3 )
#define MSG_LOG4( exp, param1, param2, param3, param4 ) MsgLog(__FILE__,__LINE__, exp, param1, param2, param3, param4 )
#define MSG_LOG5( exp, param1, param2, param3, param4, param5 ) MsgLog(__FILE__,__LINE__, exp, param1, param2, param3, param4, param5 )
#define MSG_LOG6( exp, param1, param2, param3, param4, param5, param6 ) MsgLog(__FILE__,__LINE__, exp, param1, param2, param3, param4, param5, param6 )
#define MSG_LOG7( exp, param1, param2, param3, param4, param5, param6, param7 ) MsgLog(__FILE__,__LINE__, exp, param1, param2, param3, param4, param5, param6, param7 )
#define MSG_LOG8( exp, param1, param2, param3, param4, param5, param6, param7, param8 ) MsgLog(__FILE__,__LINE__, exp, param1, param2, param3, param4, param5, param6, param7 , param8 )
#else
//#define MSG_LOG( exp )
#define MSG_LOG0( exp )
#define MSG_LOG1( exp, param1 )
#define MSG_LOG2( exp, param1, param2 )
#define MSG_LOG3( exp, param1, param2, param3 )
#define MSG_LOG4( exp, param1, param2, param3, param4 )
#define MSG_LOG5( exp, param1, param2, param3, param4, param5 )
#define MSG_LOG6( exp, param1, param2, param3, param4, param5, param6 )
#define MSG_LOG7( exp, param1, param2, param3, param4, param5, param6, param7 )
#define MSG_LOG8( exp, param1, param2, param3, param4, param5, param6, param7, param8 )
#endif


#ifdef USE_ERR_LOG
//#define ERR_LOG( exp )			ErrLog(__FILE__,__LINE__,exp)
#define ERR_LOG0( exp )			ErrLog(__FILE__,__LINE__,exp)
#define ERR_LOG1( exp, param1 )	ErrLog(__FILE__,__LINE__, exp, param1 )
#define ERR_LOG2( exp, param1, param2 ) ErrLog(__FILE__,__LINE__, exp, param1, param2 )
#define ERR_LOG3( exp, param1, param2, param3 ) ErrLog(__FILE__,__LINE__, exp, param1, param2,param3 )
#define ERR_LOG4( exp, param1, param2, param3, param4 ) ErrLog(__FILE__,__LINE__, exp, param1, param2, param3, param4 )
#define ERR_LOG5( exp, param1, param2, param3, param4, param5 ) ErrLog(__FILE__,__LINE__, exp, param1, param2, param3, param4, param5 )
#define ERR_LOG6( exp, param1, param2, param3, param4, param5, param6 ) ErrLog(__FILE__,__LINE__, exp, param1, param2, param3, param4, param5, param6 )
#define ERR_LOG7( exp, param1, param2, param3, param4, param5, param6, param7 ) ErrLog(__FILE__,__LINE__, exp, param1, param2, param3, param4, param5, param6, param7 )
#else
//#define ERR_LOG( exp )
#define ERR_LOG0( exp )
#define ERR_LOG1( exp, param1 )
#define ERR_LOG2( exp, param1, param2 )
#define ERR_LOG3( exp, param1, param2, param3 )
#define ERR_LOG4( exp, param1, param2, param3, param4 )
#define ERR_LOG5( exp, param1, param2, param3, param4, param5 )
#define ERR_LOG6( exp, param1, param2, param3, param4, param5, param6 )
#define ERR_LOG7( exp, param1, param2, param3, param4, param5, param6, param7 )
#endif

inline void MsgLog(const char* pszFile,const long lLine,const char *sFormat, ...)
{
	if ( 0 == g_USE_MSG_LOG_FLAG )
		return ;

	char szTemp[4000];
	va_list ap;

	va_start(ap, sFormat);
	memset(szTemp, 0, sizeof(szTemp));
#ifdef WIN32
	_vsnprintf(szTemp,sizeof(szTemp)-1,sFormat, ap);
#else
	vsnprintf(szTemp,sizeof(szTemp),sFormat, ap);
#endif
	va_end(ap);

//	printf("%s\n", szTemp);
	msg_log.Write("%s[file:%s][line:%ld].",szTemp, pszFile, lLine);
}



inline void ErrLog(const char* pszFile,const long lLine,const char *sFormat, ...)
{
	if ( 0 == g_USE_ERR_LOG_FLAG )
		return ;

	char szTemp[4000];
	va_list ap;

	va_start(ap, sFormat);
	memset(szTemp, 0, sizeof(szTemp));
#ifdef WIN32
	_vsnprintf(szTemp,sizeof(szTemp)-1,sFormat, ap);
#else
	vsnprintf(szTemp,sizeof(szTemp),sFormat, ap);
#endif
	va_end(ap);

//	printf("%s\n", szTemp);
	err_log.Write("%s[file:%s][line:%ld].",szTemp, pszFile, lLine);

}


#endif
