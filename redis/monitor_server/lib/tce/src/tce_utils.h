
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


#ifndef __TCE_UTILS_H__
#define __TCE_UTILS_H__

#include <stdarg.h>
#include <time.h>
#ifdef WIN32
#include <atlbase.h>
//#include<mmsystem.h>   
//#pragma   comment(lib,"winmm.lib")   
#else
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iconv.h>
#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <netinet/in.h>
#include <net/if_arp.h> 
#include <net/if.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif


#include <string>
#include <map>
#include <vector>
#include <set>
#include <sstream>
#include <iostream>

#ifdef WIN32
inline uint64_t atoll(const char* pData){
	return static_cast<uint64_t>(atol(pData));
}
#endif

namespace tce{

#define TCE_IS_GBK_CHAR(c1, c2) \
	((unsigned char)c1 >= 0x81 \
	&& (unsigned char)c1 <= 0xfe \
	&& (unsigned char)c2 >= 0x40 \
	&& (unsigned char)c2 <= 0xfe \
	&& (unsigned char)c2 != 0x7f) 

#define TCE_UTF8_LENGTH(c) \
	((unsigned char)c <= 0x7f ? 1 : \
	((unsigned char)c & 0xe0) == 0xc0 ? 2: \
	((unsigned char)c & 0xf0) == 0xe0 ? 3: \
	((unsigned char)c & 0xf8) == 0xf0 ? 4: 0)

inline void xsnprintf(char* szBuffer,const uint32_t size,const char *sFormat, ...)
{
	va_list ap;

	va_start(ap, sFormat);
#ifdef WIN32
	_vsnprintf_s(szBuffer,size-1,_TRUNCATE, sFormat, ap);
	szBuffer[size-1]='\0';
#else
	vsnprintf(szBuffer,size,sFormat, ap);
#endif
	va_end(ap);
}

inline void xsleep(const uint32_t dwMilliseconds)
{
#ifdef WIN32
	::Sleep(dwMilliseconds);
#else
	usleep(dwMilliseconds*1000);
#endif
}

inline time_t GetTickCount()
{
#if defined(WIN32)
	return ::GetTickCount();
//	return ::timeGetTime();
#elif defined(_POSIX_THREADS)		
	timeval tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec * 1000 + tv.tv_usec/1000; 
	//return clock() / (CLOCKS_PER_SEC / 1000);
#endif		
}
	

inline std::string InetNtoA(const uint32_t dwIp){
	struct in_addr in;
	in.s_addr = htonl(dwIp);
	return std::string(inet_ntoa(in));
}

inline uint32_t InetAtoN(const std::string& addr)
{
	struct in_addr sinaddr;
	if(inet_pton(AF_INET, addr.c_str(), &sinaddr) <= 0)	//error
		return 0;
	return ntohl(sinaddr.s_addr);
}

	template <class T> 
		std::string ToStr(const T &t)
	{
		std::stringstream stream;
		stream<<t;
		return stream.str();
	}

	void init_daemon();

	bool set_file_limit(const uint32_t iLimit);
	bool set_core_limit(const uint32_t iLimit);

	unsigned long getSystemMemory();

	std::string FormUrlEncode(const std::string& sSrc);
	std::string FormUrlDecode(const std::string& sSrc);
	std::string CharToHex(char c);
	inline char HexToChar(char first, char second){
		int32_t digit;
		digit = (first >= 'A' ? ((first & 0xDF) - 'A') + 10 : (first - '0'));
		digit *= 16;
		digit += (second >= 'A' ? ((second & 0xDF) - 'A') + 10 : (second - '0'));
		return static_cast<char>(digit);
	}
	std::string& TrimString(std::string& sSource);
	std::string CutString(const std::string& sData,const unsigned short wLen, const bool bUtf8);
	std::string CutString2(const std::string& sData,const unsigned short wLen, const bool bUtf8);

	std::string HexShow(const unsigned char* pszData, const int32_t iLen, const int32_t iLineCount=0);
	inline std::string HexShow(const char* pszData, const int32_t iLen, const int32_t iLineCount=0){
		return HexShow(reinterpret_cast<const unsigned char*>(pszData), iLen, iLineCount);
	}
	inline std::string HexShow(const std::string& sData, const int32_t iLineCount=0)
	{
		return HexShow(reinterpret_cast<const unsigned char*>(sData.data()), (int32_t)sData.size(), iLineCount);
	}

	std::string HexShow2(const unsigned char* pszData, const int32_t iLen, const int32_t iLineCount=0);
	inline std::string HexShow2(const char* pszData, const int32_t iLen, const int32_t iLineCount=0){
		return HexShow2(reinterpret_cast<const unsigned char*>(pszData), iLen, iLineCount);
	}
	inline std::string HexShow2(const std::string& sData, const int32_t iLineCount=0)
	{
		return HexShow2(reinterpret_cast<const unsigned char*>(sData.data()), (int32_t)sData.size(), iLineCount);
	}



	void SplitWeak( std::vector<std::string > & v, const std::string & src, const std::string & tok, const bool trim = false, const std::string & null_subst = "");
	void SplitStrong( std::vector<std::string > & v, const std::string & src, const std::string & delimit, const std::string & null_subst = "");

	//获取网卡IP地址(eth0/eth1/tunl0 等)
	std::string GetNetCardIP (const std::string &sNetCardName) ;
	int GetAllLocalIP(std::set<std::string>& ips);

#ifndef WIN32
	typedef std::map<std::string, struct stat> MAP_STR_STAT;
	bool ScanDir(const std::string& sPath, std::vector<std::string>& vecFiles, std::string& sErrMsg);
	bool ScanDir(const std::string& sPath, MAP_STR_STAT& mapFiles, std::string& sErrMsg);
#endif

	//获取格林时间
	std::string getGMTDate(const time_t& cur);

	time_t gmt2time(const char *str);

	//added by alex 2009-10-23 , 短格式20091023093601，长格式2009-10-23 09:36:01
	std::string GetDateTimeStr(time_t tTime, bool bLongFormat=true);
	std::string GetCurDateTimeStr(bool bLongFormat=true);
	time_t GetDateTime(const std::string &sDateTime);

	inline std::string GetTimeStr(const time_t dwTime){
		struct tm curr;
		curr = *localtime(&dwTime);
		char szDate[50]; 
		tce::xsnprintf(szDate, sizeof(szDate)-1, "%04d-%02d-%02d %02d:%02d:%02d", curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday, curr.tm_hour, curr.tm_min, curr.tm_sec);
		return std::string(szDate); 
	}

	template<int32_t v> 
	struct Int2Type {
		enum {type = v};
	};

	uint32_t CRC32(const char* ptr,const uint32_t dwSize);
	

/*
	bool base16decode(const char* pszSign, const int32_t iLen, unsigned char* pszOut , int32_t& iOutLen);
	std::string  base16decode(const char* pszSign, const int32_t iLen);
	std::string base16encode(const char* pszSign, const int32_t iLen);
*/	
	
	bool Base16Decode(const char* hex, const size_t hexlen, char* bin, size_t& binlen);
	bool Base16Encode(const char* bin, const size_t binlen, char* hex, size_t& hexlen);
	inline std::string Base16Decode(const char* hex, const size_t hexlen){
		std::string sResult;
		sResult.resize(hexlen/2);
		size_t nSize = hexlen/2;
		Base16Decode(hex, hexlen, (char*)sResult.data(), nSize);
		return sResult;
	}
	inline std::string Base16Encode(const char* bin, const size_t binlen){
		std::string sResult;
		sResult.resize(binlen*2);
		size_t nSize = binlen*2;
		Base16Encode(bin, binlen, (char*)sResult.data(), nSize);
		return sResult;
	}
	
	const uint32_t SEC_PER_MIN = 60;
	const uint32_t MIN_PER_HOUR = 60;
	const uint32_t HOUR_PER_DAY = 24;

	const uint32_t SEC_PER_HOUR = MIN_PER_HOUR*SEC_PER_MIN;
	const uint32_t SEC_PER_DAY = HOUR_PER_DAY*SEC_PER_HOUR;
	const uint32_t MIN_PER_DAY = HOUR_PER_DAY*MIN_PER_HOUR;

	class CLocalTime
	{
	public:
		CLocalTime(time_t tTime = ::time(NULL)): m_tTime(tTime)
		{
			localtime_r(&m_tTime, &m_stTM);
		}

		~CLocalTime()	{}

		void SetTime(time_t tTime = ::time(NULL))
		{
			m_tTime = tTime;
			localtime_r(&m_tTime, &m_stTM);
		}

		time_t time(void) const
		{
			return m_tTime;
		}

		size_t year(void) const
		{
			return m_stTM.tm_year+1900;             /* m_stTM.tm_year中存放的是从1900年至今的年数 */
		}

		size_t month(void) const
		{
			return m_stTM.tm_mon+1;                 /* m_stTM.tm_mon的范围是0~11 */
		}

		size_t day(void) const
		{
			return m_stTM.tm_mday;
		}

		size_t hour(void) const
		{
			return m_stTM.tm_hour;
		}

		size_t minute(void) const
		{
			return m_stTM.tm_min;
		}

		size_t second(void) const
		{
			return m_stTM.tm_sec;
		}

	private:
		time_t m_tTime;
		struct tm m_stTM;
	};	
	class CTimeAnalysor
	{
	public:
		CTimeAnalysor(time_t tTime = ::time(NULL)): m_tTime(tTime), oLT(tTime)	{}
		~CTimeAnalysor()	{}

		void SetTime(time_t tTime = ::time(NULL))
		{
			m_tTime = tTime;
			oLT.SetTime(m_tTime);
		}

		time_t GetBeginOfDay(uint32_t dwDayAgo = 0) const
		{
			return (m_tTime-SEC_PER_HOUR*oLT.hour()-SEC_PER_MIN*oLT.minute()-oLT.second()-SEC_PER_DAY*dwDayAgo);
		}

		time_t GetEndOfDay(uint32_t dwDayAgo = 0) const
		{
			return (GetBeginOfDay(dwDayAgo)+SEC_PER_DAY);
		}

		time_t GetValue(void) const
		{
			return m_tTime;
		}

		size_t GetDayValue(void) const
		{
			return oLT.year() * 10000 + oLT.month() * 100 + oLT.day();
		}

		size_t GetMinIdxOfHour(void) const
		{
			return oLT.minute();
		} 

		size_t GetMinIdxOfDay(void) const
		{
			return oLT.minute() + oLT.hour() * MIN_PER_HOUR;
		}

		size_t GetHourIdxOfDay(void) const
		{
			return oLT.hour();
		}

	private:
		time_t m_tTime;
		CLocalTime oLT;
	};

	class CTimeCost
	{
	public:
	        CTimeCost()
	        {
	                reset();
	        }

	        ~CTimeCost()
	        {
	        }

	        void reset()
	        {
	                gettimeofday(&m_tv, NULL);
	                m_tStartTime = m_tv.tv_sec * 1000 + m_tv.tv_usec / 1000;
	        }

	        time_t value()
	        {
	                gettimeofday(&m_tv, NULL);
	                m_tEndTime = m_tv.tv_sec * 1000 + m_tv.tv_usec / 1000;
	                return (m_tEndTime - m_tStartTime);
	        }

	private:
	        time_t m_tStartTime;
	        time_t m_tEndTime;
	        timeval m_tv;
	};
};

#endif
