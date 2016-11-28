
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


#include "tce_utils.h"

using namespace std;

namespace tce{

void ignore_pipe(void)
	{
#ifndef WIN32
		struct sigaction sig;
		sig.sa_handler = SIG_IGN;
		sig.sa_flags = 0;
		sigemptyset(&sig.sa_mask);
		sigaction(SIGPIPE,&sig,NULL);
#endif
	}

void init_daemon()
{
#ifndef _WIN32
		pid_t pid;

		if ((pid = fork()) != 0)
			exit(0);

		setsid();

		signal(SIGINT,  SIG_IGN);
		signal(SIGHUP,  SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
		signal(SIGTERM, SIG_IGN);

		signal(SIGIO,   SIG_IGN);

		ignore_pipe();

		if ((pid = fork()) != 0)
			exit(0);

		//chdir("/");  去掉这个, 服务程序所占用的目录,不允许删除。如果加了chdir 则允许。
		umask(0);
#else
#endif
}

bool set_file_limit(const uint32_t iLimit)
{
	bool bOK = false;
#ifndef WIN32
	struct rlimit rlim = {0};
	if(getrlimit((__rlimit_resource_t)RLIMIT_OFILE, &rlim) == 0)
	{
		//		int cur = rlim.rlim_cur;
		if(rlim.rlim_cur < iLimit)
		{
			rlim.rlim_cur = iLimit;
			rlim.rlim_max = iLimit;
			if(setrlimit((__rlimit_resource_t)RLIMIT_OFILE, &rlim) == 0)
			{
				bOK = true;
			}
		}
		else
		{
			bOK = true;
		}
	}
#else
	bOK = true;
#endif
	return bOK;
}

bool set_core_limit(const uint32_t iLimit)
{
	bool bOK = false;
#ifndef WIN32
	struct rlimit rlim = {0};
	if(getrlimit((__rlimit_resource_t)RLIMIT_CORE, &rlim) == 0)
	{
		//		int cur = rlim.rlim_cur;
		if(rlim.rlim_cur != iLimit)
		{
			rlim.rlim_cur = iLimit;
			rlim.rlim_max = iLimit;
			if(setrlimit((__rlimit_resource_t)RLIMIT_CORE, &rlim) == 0)
			{
				bOK = true;
			}
		}
		else
		{
			bOK = true;
		}
	}
#else
	bOK = true;
#endif
	return bOK;
}

unsigned long getSystemMemory()
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);
	return pages * page_size;
}


std::string FormUrlEncode(const std::string &sSrc)
{
	std::string sResult;
	for(std::string::const_iterator iter = sSrc.begin(); iter != sSrc.end(); ++iter) {
		switch(*iter) {
		case ' ':
			sResult.append(1, '+');
		  break;
			// alnum
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
		case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
		case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
		case 'V': case 'W': case 'X': case 'Y': case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
		case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
		case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
		case 'v': case 'w': case 'x': case 'y': case 'z':
		case '0': case '1': case '2': case '3': case '4': case '5': case '6':
		case '7': case '8': case '9':
			// mark
		case '-': case '_': case '.': case '!': case '~': case '*': case '\'': 
		case '(': case ')':
			sResult.append(1, *iter);
			break;
			// escape
		default:
			sResult.append(1, '%');
			sResult.append(CharToHex(*iter));
			break;
		}
	}

	return sResult;
}


std::string FormUrlDecode(const std::string& sSrc)
{
	std::string sResult;
	const char* pszSrc = sSrc.c_str();
	char c;
	size_t iSize = sSrc.size();
	for (size_t i=0;i<iSize; ++i)
	{
		switch(*(pszSrc+i)) {
		case '+':
		sResult.append(1, ' ');
		break;
		case '%':
		// assume well-formed input
		++i;
		if( 'u' != *(pszSrc+i) )
		{
			c = *(pszSrc+i);
			++i;
			sResult.append(1, HexToChar(c, *(pszSrc+i)));
		}
		else
		{
			char u[2];
			u[1] = HexToChar(*(pszSrc+i+1), *(pszSrc+i+2));
			u[0] = HexToChar(*(pszSrc+i+3), *(pszSrc+i+4));
#ifdef WIN32
			char* pa;
			USES_CONVERSION;
			pa = W2A((wchar_t*)u);
			sResult.append(pa,2);
#else
			char szOut[4];
			char* pIn = u;
			char* pOut = szOut;
			size_t in=2,out=4;
			iconv_t fdIconv = iconv_open("GBK","UNICODE"); 
			iconv(fdIconv,&pIn, &in,&pOut,&out);
			iconv_close(fdIconv);
			sResult.append(szOut,2);
#endif
			i +=4;
		}

		break;
		default:
		sResult.append(1, *(pszSrc+i));
		break;
		}
	}
/*
	for(string::const_iterator iter = sSrc.begin(); iter != sSrc.end(); ++iter) {
	switch(*iter) {
	case '+':
	sResult.append(1, ' ');
	break;
	case '%':
	// assume well-formed input
	c = *++iter;
	sResult.append(1, HexToChar(c, *++iter));
	break;
	default:
	sResult.append(1, *iter);
	break;
	}
	}
*/
	return sResult;
}

std::string CharToHex(char c)
{
	std::string sResult;
  char first, second;

  first = (c & 0xF0) / 16;
  first += first > 9 ? 'A' - 10 : '0';
  second = c & 0x0F;
  second += second > 9 ? 'A' - 10 : '0';

  sResult.append(1, first);
  sResult.append(1, second);
  
  return sResult;
}

std::string HexShow(const unsigned char* sStr, const int32_t iLen, const int32_t iLineCount)
{
	register int32_t iCount;
	std::string sResult;
	char szTmp[10];
	for (iCount = 0; iCount < iLen; iCount++) {
		//if (iFlag && sStr[iCount] > 0x1f) 
		//{
		//	snprintf(szTmp, sizeof(szTmp), "%2c ", sStr[iCount]);
		//	sResult += szTmp;
		//}
		//else 
		{
			tce::xsnprintf(szTmp, sizeof(szTmp), "%.2x", (unsigned char)sStr[iCount]);
			sResult += szTmp;
		}
		if (iLineCount > 0 && (iCount+1) % iLineCount == 0 )
			sResult += "\n";
	}

	return sResult;
}

std::string HexShow2(const unsigned char* pszData, const int32_t iLen, const int32_t iLineCount)
{
	register int32_t iCount;
	std::string sResult;
	char szTmp[10];
	for (iCount = 0; iCount < iLen; iCount++) {
		if ( isprint(pszData[iCount]) )
		{
			sResult += pszData[iCount];
		} 
		else
		{
			tce::xsnprintf(szTmp, sizeof(szTmp), "%.2x", (unsigned char)pszData[iCount]);
			sResult += szTmp;
		}

		if (iLineCount > 0 && (iCount+1) % iLineCount == 0 )
			sResult += "\n";
	}

	return sResult;
}

static const char *days[] = {"Sun", "Mon", "Tue",  "Wed",  "Thu",  "Fri",  "Sat"};
static const char *months[] = {"Jan",  "Feb",  "Mar",  "Apr",  "May",  "Jun",  "Jul",  "Aug", "Sep",  "Oct",  "Nov",  "Dec"};

std::string getGMTDate(const time_t& cur)
{
    char buf[256];
    struct tm  *gt = gmtime(&cur);
	tce::xsnprintf(buf, sizeof(buf),"%s, %02d %s %04d %02d:%02d:%02d GMT",
             days[gt->tm_wday],
             gt->tm_mday,
             months[gt->tm_mon],
             gt->tm_year + 1900,
             gt->tm_hour,
             gt->tm_min,
             gt->tm_sec);
    return buf;
}

time_t gmt2time(const char *str) {
#define BAD	0
#define GC	(c=*tstr++)
	const unsigned char *tstr = reinterpret_cast<const unsigned char*>(str);
	char c;
	int year = -1, month, day = -1, sec;

	while(GC != ' ')
		if(!c) return BAD;
	if(isdigit(GC)) {
		day = c - '0';
		if(isdigit(GC)) {
			day = day * 10 + c - '0';
			GC;
		}
		if(c!=' ' && c!='-') return BAD;
		GC;
	}
	switch(c) {
		case 'A': /*Apr,Aug*/
			GC;
			if(c=='p') {
				if(GC != 'r') return BAD;
				month = 2; break;
			} else if(c=='u') {
				if(GC != 'g') return BAD;
				month = 6; break;
			}
			return BAD;
		case 'D': /*Dec*/
			if(GC != 'e') return BAD;
			if(GC != 'c') return BAD;
			month = 10; break;
		case 'F': /*Feb*/
			if(GC != 'e') return BAD;
			if(GC != 'b') return BAD;
			month = 12; break;
		case 'J': /*Jan,Jun,Jul*/
			GC;
			if(c=='a') {
				if(GC != 'n') return BAD;
				month = 11; break;
			} else if(c != 'u') {
				return BAD;
			}
			GC;
			if(c == 'n') {
				month = 4; break;
			} else if(c=='l') {
				month = 5; break;
			}
			return BAD;
		case 'M': /*Mar,May*/
			if(GC != 'a') return BAD;
			GC;
			if(c=='r') {
				month = 1; break;
			} else if(c=='y') {
				month = 3; break;
			}
			return BAD;
		case 'N': /*Nov*/
			if(GC != 'o') return BAD;
			if(GC != 'v') return BAD;
			month = 9; break;
		case 'O': /*Oct*/
			if(GC != 'c') return BAD;
			if(GC != 't') return BAD;
			month = 8; break;
		case 'S': /*Sep*/
			if(GC != 'e') return BAD;
			if(GC != 'p') return BAD;
			month = 7; break;
		default: return BAD;
	}
	GC; if(c!=' ' && c!='-') return BAD;
	GC; if(c==' ') GC;
	if(day==-1) {
		day = c - '0';
		if(isdigit(GC)) {
			day = day * 10 + c - '0';
			GC;
		}
	} else {
		year = c - '0';
		if(isdigit(GC)) {
			year = year * 10 + c - '0';
			if(isdigit(GC)) {
				year = year * 10 + c - '0';
				if(isdigit(GC)) {
					year = year * 10 + c - '0';
					GC;
				}
			}
		}
	}
	if(c!=' ') return BAD;
	GC; if(!isdigit(c)) return BAD;
	sec = (c-'0') * 36000;
	GC; if(!isdigit(c)) return BAD;
	sec += (c-'0') * 3600;
	GC; if(c!=':') return BAD;
	GC; if(!isdigit(c)) return BAD;
	sec += (c-'0') * 600;
	GC; if(!isdigit(c)) return BAD;
	sec += (c-'0') * 60;
	GC; if(c!=':') return BAD;
	GC; if(!isdigit(c)) return BAD;
	sec += (c-'0') * 10;
	GC; if(!isdigit(c)) return BAD;
	sec += c-'0';
	GC; if(c!=' ') return BAD;
	if(year==-1) {
		if(!isdigit(GC)) return BAD;
		year = (c-'0') * 1000;
		if(!isdigit(GC)) return BAD;
		year += (c-'0') * 100;
		if(!isdigit(GC)) return BAD;
		year += (c-'0') * 10;
		if(!isdigit(GC)) return BAD;
		year += c-'0';
	}
	if(year < 70)
		year += 32;
	else if(year < 100)
		year -= 68;
	else if(year < 1970)
		return BAD;
	else if(year < 2040)
		year -= 1968;
	else
		return BAD;
	if(month > 10) year--;
	year = year * 365 + year/4 + month*520/17 + day;
	return year * 86400 + sec - 60652800;
}


std::string GetDateTimeStr(time_t tTime, bool bLongFormat/*=true*/)
{
	char s[20];
	struct tm curr = *localtime(&tTime);

	if(bLongFormat)
	{
		xsnprintf(s, sizeof(s), "%04d-%02d-%02d %02d:%02d:%02d", 
			curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday,
			curr.tm_hour, curr.tm_min, curr.tm_sec);
	}
	else
	{
		xsnprintf(s, sizeof(s), "%04d%02d%02d%02d%02d%02d", 
			curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday,
			curr.tm_hour, curr.tm_min, curr.tm_sec);
	}
				
	return std::string(s);
}

std::string GetCurDateTimeStr(bool bLongFormat/*=true*/)
{
	time_t tNow = time(NULL);
	return GetDateTimeStr(tNow, bLongFormat);
}

time_t GetDateTime(const std::string &sDateTime)
{		
	struct tm stDateTime;

	if (sDateTime.length() == 14)	//短格式
	{
		stDateTime.tm_year = atoi(sDateTime.substr(0, 4).c_str()) - 1900;
		stDateTime.tm_mon = atoi(sDateTime.substr(4, 2).c_str()) - 1;
		stDateTime.tm_mday = atoi(sDateTime.substr(6, 2).c_str());
		stDateTime.tm_hour = atoi(sDateTime.substr(8, 2).c_str());
		stDateTime.tm_min = atoi(sDateTime.substr(10, 2).c_str());
		stDateTime.tm_sec = atoi(sDateTime.substr(12, 2).c_str());
	}
	else if (sDateTime.length() == 19)	//长格式
	{
		stDateTime.tm_year = atoi(sDateTime.substr(0, 4).c_str()) - 1900;
		stDateTime.tm_mon = atoi(sDateTime.substr(5, 2).c_str()) - 1;
		stDateTime.tm_mday = atoi(sDateTime.substr(8, 2).c_str());
		stDateTime.tm_hour = atoi(sDateTime.substr(11, 2).c_str());
		stDateTime.tm_min = atoi(sDateTime.substr(14, 2).c_str());
		stDateTime.tm_sec = atoi(sDateTime.substr(17, 2).c_str());
	}
	else
	{
		return 0;
	}

	return mktime(&stDateTime);
}

/*截取指定长度的字符，可以处理中文*/
std::string CutString(const std::string& sData,const uint16_t wLen, const bool bUtf8)
{
	if(sData.size()<wLen)
		return sData;
	
	std::string sDes;
	if ( bUtf8 )
	{

		for(std::string::size_type i=0;i<sData.size();i++)
		{

			int iUtf8Size =  TCE_UTF8_LENGTH(sData[i]);
			if( (i+iUtf8Size-1) >= wLen)
			{
				break;
			}			

			if ( iUtf8Size == 1 )
			{
				sDes += sData[i];
			}
			else
			{
				if(iUtf8Size > 1)
				{
					if ( iUtf8Size+i<=sData.size() )
					{
						sDes.append(sData.data()+i, iUtf8Size);
						i += iUtf8Size-1;
					}
					//else
					//{
					//	static const unsigned short __wrong_padding_utf16 = 0xff1f;  
					//	sDes += tce_single_utf16to8(__wrong_padding_utf16);
					//	i += iUtf8Size-1;
					//}
				}
			}
		}
	}
	else
	{
		for(std::string::size_type i=0;i<sData.size();i++)
		{
			if(i>=wLen)
			{
				break;
			}
			if( (unsigned char)sData[i] >= 0x81 && (unsigned char)sData[i]<= 0xfe ) 
			{ 
				if (i+1<wLen) 
				{
					if ( TCE_IS_GBK_CHAR(sData[i], sData[i+1])  )
					{
						sDes +=sData[i];
						sDes +=sData[++i];
					}
				}
			} 
			else 
			{ 
				sDes +=sData[i];
			} 		
		}
	}
	return sDes;
}


std::string CutString2(const std::string& sData,const uint16_t wLen, const bool bUtf8)
{
	if(sData.size()<wLen)
		return sData;

	uint16_t wTmpLen = wLen;

	bool bCutFlag = false;
	
	std::string sDes;
	if ( bUtf8 )
	{

		for(std::string::size_type i=0;i<sData.size();i++)
		{

			int iUtf8Size =  TCE_UTF8_LENGTH(sData[i]);
			if( (i+iUtf8Size-1) >= wTmpLen)
			{
				bCutFlag = true;
				break;
			}			

			if ( iUtf8Size == 1 )
			{
				sDes += sData[i];
			}
			else
			{
				if(iUtf8Size > 1)
				{
					if ( iUtf8Size == 3 )
					{
						wTmpLen++;
					}
					

					if ( iUtf8Size+i<=sData.size() )
					{
						sDes.append(sData.data()+i, iUtf8Size);
						i += iUtf8Size-1;
					}
					//else
					//{
					//	static const unsigned short __wrong_padding_utf16 = 0xff1f;  
					//	sDes += tce_single_utf16to8(__wrong_padding_utf16);
					//	i += iUtf8Size-1;
					//}
				}
			}
		}
	}
	else
	{
		for(std::string::size_type i=0;i<sData.size();i++)
		{
			if(i>=wTmpLen)
			{
				break;
			}
			if( (unsigned char)sData[i] >= 0x81 && (unsigned char)sData[i]<= 0xfe ) 
			{ 
				if (i+1<wLen) 
				{
					if ( TCE_IS_GBK_CHAR(sData[i], sData[i+1])  )
					{
						sDes +=sData[i];
						sDes +=sData[++i];
					}
				}
			} 
			else 
			{ 
				sDes +=sData[i];
			} 		
		}
	}
	if ( bCutFlag )
	{
		sDes += "...";
	}
	
	return sDes;
}






#ifndef WIN32
bool ScanDir(const std::string& sPath, MAP_STR_STAT& mapFiles, std::string& sErrMsg)
{
	struct stat s; 
	DIR *dir; 
	struct dirent *dt; 

	std::string sTmpPath(sPath);

	if(lstat(sTmpPath.c_str(), &s) < 0){ 
		char szTmp[1024];
		snprintf(szTmp, sizeof(szTmp), "lstat<file=%s> error:%s", sTmpPath.c_str(), strerror(errno));
		sErrMsg = szTmp;
		return false;
	} 

	if(S_ISDIR(s.st_mode)){ 

		if (sTmpPath[sTmpPath.size()-1] != '/')
		{
			sTmpPath += '/';
		}

		if((dir = opendir(sTmpPath.c_str())) == NULL){ 
			char szTmp[1024];
			snprintf(szTmp, sizeof(szTmp), "opendir<file=%s> error:%s", sTmpPath.c_str(), strerror(errno));
			sErrMsg = szTmp;
			return false;
		} 

		while((dt = readdir(dir)) != NULL){ 
			if(dt->d_name[0] == '.'){ 
				continue; 
			} 


			if (!ScanDir(sTmpPath+dt->d_name, mapFiles, sErrMsg))
			{
				closedir(dir);
				return false;
			}
		} 
		closedir(dir);
	}else{ 
		mapFiles.insert(MAP_STR_STAT::value_type(sTmpPath,s));
	} 

	return true;
}


bool ScanDir(const std::string& sPath, std::vector<std::string>& vecFiles, std::string& sErrMsg)
{
	struct stat s; 
	DIR *dir; 
	struct dirent *dt; 

	std::string sTmpPath(sPath);

	if(lstat(sTmpPath.c_str(), &s) < 0){ 
		char szTmp[1024];
		snprintf(szTmp, sizeof(szTmp), "lstat<file=%s> error:%s", sTmpPath.c_str(), strerror(errno));
		sErrMsg = szTmp;
		return false;
	} 

	if(S_ISDIR(s.st_mode)){ 

		if((dir = opendir(sTmpPath.c_str())) == NULL){ 
			char szTmp[1024];
			snprintf(szTmp, sizeof(szTmp), "opendir<file=%s> error:%s", sTmpPath.c_str(), strerror(errno));
			sErrMsg = szTmp;
			return false;
		} 

		while((dt = readdir(dir)) != NULL){ 
			if(dt->d_name[0] == '.'){ 
				continue; 
			} 


			if (!ScanDir(sTmpPath+"/"+dt->d_name, vecFiles, sErrMsg))
			{
				closedir(dir);
				return false;
			}
			closedir(dir);
		} 

	}else{ 
		vecFiles.push_back(sTmpPath);
	} 

	return true;
}
#endif


//去掉字符串2头的空格
std::string& TrimString(std::string& sSource)
{
	if (sSource.size()==0)
		return sSource;		
	
	std::string sDest = "";

	size_t i = 0;
	for(; i < sSource.size() && 
		(sSource[i] == ' '  ||
		sSource[i] == '\r'  || 
		sSource[i] == '\n'  || 
		sSource[i] == '\t'); i++);

	size_t j = sSource.size() - 1;
	if(j > 0)
	{
		for(; j > 0 && 
			(sSource[j] == ' '   ||
			sSource[j] == '\r'  || 
			sSource[j] == '\n'  || 
			sSource[j] == '\t')
			; j--);
	}

	if(j >= i)
	{
		sDest = sSource.substr(i, j-i+1);
		sSource = sDest;
	}
	else
		sSource = "";
	return sSource;
}

/*
bool base16decode(const char* pszSign, const int32_t iLen, unsigned char* pszOut , int32_t& iOutLen)
{
	if ( iOutLen < iLen/2 )
		return false;

	int32_t iTemp;
	for(int32_t i=0; i<iLen; i+=2)
	{
	    sscanf(pszSign+i, "%02X", &iTemp);
	    *(pszOut+i/2) = iTemp;
	}
	iOutLen = iLen/2;
	return true;
}

std::string  base16decode(const char* pszSign, const int32_t iLen)
{
	std::string sTmp;
	sTmp.resize(iLen/2);

	int32_t iTemp;
	for(int32_t i=0; i<iLen; i+=2)
	{
		sscanf(pszSign+i, "%02X", &iTemp);
		*((unsigned char*)sTmp.data()+i/2) = iTemp;
	}
	return sTmp;
}

std::string base16encode(const char* pszSign, const int32_t iLen)
{
	std::string sTmp;
	char szTmp[10];
	for ( int32_t i=0; i<iLen; ++i )
	{
		tce::xsnprintf(szTmp, sizeof(szTmp), "%02X", (unsigned char)pszSign[i]);
		sTmp += szTmp;
	}
	return sTmp;
}
*/

//2009-02-27 alex ，tok为分隔符集合,trim指示是否保留空串，默认为保留  
void SplitWeak(std::vector<std::string> &v, 
				const std::string &src,
				const std::string &tok, 
				const bool trim /*= false*/,
				const std::string &null_subst /*=""*/)   
{   
	v.clear();
	if( src.empty() || tok.empty() ) 
		return;   

	std::string::size_type pre_index = 0, index = 0, len = 0;   
	while( (index = src.find_first_of(tok, pre_index)) != std::string::npos )   
	{   
		if( (len = index-pre_index)!= 0 )   
			v.push_back(src.substr(pre_index, len));   
		else if(trim==false)   
			v.push_back(null_subst);
		pre_index = index+1;   
	}
	
	std::string endstr = src.substr(pre_index);   
	if( trim==false ) 
		v.push_back( endstr.empty()? null_subst:endstr );   
	else if( !endstr.empty() ) 
		v.push_back(endstr);
	
}   

//2009-02-27 alex 使用一个完整的串delimit（而不是其中的某个字符）来严格分割src串
void SplitStrong(std::vector<std::string> &v, 
				const std::string &src, 
				const std::string &delimit, 
				const std::string &null_subst /*=""*/)
{   
	v.clear();
	if( src.empty() || delimit.empty() ) 
		return;   

	std::string::size_type deli_len = delimit.size();   
	std::string::size_type index = 0;
	std::string::size_type last_search_position = 0;   
	while( (index=src.find(delimit, last_search_position)) != std::string::npos )
	{   
		if(index==last_search_position)   
			v.push_back(null_subst);   
		else  
			v.push_back( src.substr(last_search_position, index-last_search_position) );
		
		last_search_position = index + deli_len;   
	}
	
	std::string last_one = src.substr(last_search_position);
	v.push_back( last_one.empty() ? null_subst:last_one ); 
	
} 



std::string GetNetCardIP (const std::string &sNetCardName) 
{ 
	static const long MAXINTERFACES = 16; 
	static const std::string sNull("");
	
	register int32_t fd, intrface; 
	struct ifreq buf[MAXINTERFACES]; 
	struct ifconf ifc; 

	if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		return sNull;
	}
	
	ifc.ifc_len = sizeof buf; 
	ifc.ifc_buf = (caddr_t) buf; 
	if (ioctl (fd, SIOCGIFCONF, (char *) &ifc)) 
	{ 
		close (fd); 
		return sNull;
	}
	
	intrface = ifc.ifc_len / sizeof (struct ifreq); 
	while (intrface-- > 0) 
	{ 
		if(sNetCardName != buf[intrface].ifr_name)
		{
			continue;
		}

		/*Jugde whether the net card status is promisc  */ 
		if (!(ioctl (fd, SIOCGIFFLAGS, (char *) &buf[intrface])))
		{ 
			if (buf[intrface].ifr_flags & IFF_PROMISC) 
			{ 
				//std::cout << "the interface is PROMISC" << endl;
			} 
		} 
		else 
		{ 
			std::cout << "ioctl device " << buf[intrface].ifr_name << " promisc err!" << endl;
		} 

		/*Get IP of the net card */ 
		if (!(ioctl (fd, SIOCGIFADDR, (char *) &buf[intrface]))) 
		{ 
			close (fd); 
			return inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr);
		} 
		else
		{ 
 			std::cout << "ioctl device " << buf[intrface].ifr_name << " addr err!"<< endl;
			close (fd); 
			return sNull;
		} 

	} 

	close (fd); 
	return sNull;
}


int GetAllLocalIP(std::set<std::string>& ips)
{
	static const long MAXINTERFACES = 16; 
	int iSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (iSocket <= 0)
	{
		return -1;
	}

	struct ifconf stIfc;
	struct ifreq stIfcElem[MAXINTERFACES];

	stIfc.ifc_len = sizeof(stIfcElem);
	stIfc.ifc_buf = (caddr_t) stIfcElem;

	ips.clear();
	if ( ioctl(iSocket, SIOCGIFCONF, (char *)&stIfc) >= 0 ) 
	{
		int iInterfaceNum = stIfc.ifc_len / sizeof (struct ifreq);
		while (iInterfaceNum--)
		{
			if ( ioctl(iSocket, SIOCGIFADDR, (char*) &stIfcElem[iInterfaceNum]) >= 0 )
			{
				string sIP = inet_ntoa( ((struct sockaddr_in*)(&stIfcElem[iInterfaceNum].ifr_addr))->sin_addr);
				ips.insert(sIP);
			}
			else
			{
				close(iSocket);
				return -3;
			}
		}
	}
	else
	{
		close(iSocket);
		return -2;
	}

	close(iSocket);
	return 0;
}

////////////////////////////////////////////////////////////////
// 计算字符串的CRC32值
// 参数：欲计算CRC32值字符串的首地址和大小
// 返回值: 返回CRC32值

uint32_t CRC32(const char* ptr,const uint32_t dwSize)
{
	static bool bCreateTable = false;
	static uint32_t crcTable[256];
	uint32_t crcTmp1;

	//动态生成CRC-32表
	if ( !bCreateTable )
	{
		for (int32_t i=0; i<256; i++)
		{
			crcTmp1 = i;
			for (int32_t j=8; j>0; j--)
			{
				if (crcTmp1&1) crcTmp1 = (crcTmp1 >> 1) ^ 0xEDB88320L;
				else crcTmp1 >>= 1;
			}

			crcTable[i] = crcTmp1;
		}
		bCreateTable = true;
	}

	//计算CRC32值
	uint32_t crcTmp2= 0xFFFFFFFF;
	uint32_t dwTmpSize = dwSize;
	while(dwTmpSize--)
	{
		crcTmp2 = ((crcTmp2>>8) & 0x00FFFFFF) ^ crcTable[ (crcTmp2^(*ptr)) & 0xFF ];
		ptr++;
	}

	return (crcTmp2^0xFFFFFFFF);
}

bool Base16Decode(const char* hex, const size_t hexlen, char* bin, size_t& binlen)
{
	char c, s;
	size_t i = 0;

	while(i < binlen && *hex) {
		s = 0x20 | (*hex++);
		if(s >= '0' && s <= '9')
			c = s - '0';
		else if(s >= 'a' && s <= 'f')
			c = s - 'a' + 10;
		else
			break;

		c <<= 4;
		s = 0x20 | (*hex++);
		if(s >= '0' && s <= '9')
			c += s - '0';
		else if(s >= 'a' && s <= 'f')
			c += s - 'a' + 10;
		else
			break;
		bin[i++] = c;
	}
	if(i<binlen) bin[i] = '\0';
	binlen = i;
	return true;
}

bool Base16Encode(const char* bin, const size_t binlen, char* hex, size_t& hexlen)
{
	if ( hexlen < binlen*2 )	return false;
	
	for(size_t i=0; i<binlen; i++) {
		*hex++ = "0123456789abcdef"[((unsigned char *)bin)[i] >> 4];
		*hex++ = "0123456789abcdef"[((unsigned char *)bin)[i] & 15];
	}
	if ( hexlen > binlen*2 ) *hex = '\0';
	hexlen = binlen*2;
	return true;
}

} ;


