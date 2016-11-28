
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



#include "wbl_comm.h"
#include <errno.h>
#include <string.h>

wbl::comm_error::comm_error(const string& s):logic_error(s){}

void wbl::Daemon()
{
	pid_t pid;

	if ((pid = fork()) != 0) { exit(0); }

	setsid();
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	// ignore_pipe();
	struct sigaction sig;
	bzero(&sig, sizeof(sig));
	sig.sa_handler = SIG_IGN;
	sig.sa_flags = 0;
	sigemptyset(&sig.sa_mask);
	sigaction(SIGPIPE, &sig, NULL);

	if ((pid = fork()) != 0) 	{ exit(0);	}

	//chdir("/");

	umask(0);

	for (int i = 3; i < 64; i++) 
		close(i);
}

//phorix优化b2s，用空间换时间
static char c_b2s[256][4]={"00","01","02","03","04","05","06","07","08","09","0a","0b","0c","0d","0e","0f","10","11","12","13","14","15","16","17","18","19","1a","1b","1c","1d","1e","1f","20","21","22","23","24","25","26","27","28","29","2a","2b","2c","2d","2e","2f","30","31","32","33","34","35","36","37","38","39","3a","3b","3c","3d","3e","3f","40","41","42","43","44","45","46","47","48","49","4a","4b","4c","4d","4e","4f","50","51","52","53","54","55","56","57","58","59","5a","5b","5c","5d","5e","5f","60","61","62","63","64","65","66","67","68","69","6a","6b","6c","6d","6e","6f","70","71","72","73","74","75","76","77","78","79","7a","7b","7c","7d","7e","7f","80","81","82","83","84","85","86","87","88","89","8a","8b","8c","8d","8e","8f","90","91","92","93","94","95","96","97","98","99","9a","9b","9c","9d","9e","9f","a0","a1","a2","a3","a4","a5","a6","a7","a8","a9","aa","ab","ac","ad","ae","af","b0","b1","b2","b3","b4","b5","b6","b7","b8","b9","ba","bb","bc","bd","be","bf","c0","c1","c2","c3","c4","c5","c6","c7","c8","c9","ca","cb","cc","cd","ce","cf","d0","d1","d2","d3","d4","d5","d6","d7","d8","d9","da","db","dc","dd","de","df","e0","e1","e2","e3","e4","e5","e6","e7","e8","e9","ea","eb","ec","ed","ee","ef","f0","f1","f2","f3","f4","f5","f6","f7","f8","f9","fa","fb","fc","fd","fe","ff"};

unsigned wbl::s2u(const string &s) throw (wbl::comm_error)
{
	errno = 0;
	unsigned u = strtoul(s.c_str(),NULL,10);
	if(errno != 0) {
		throw wbl::comm_error(string("wbl::s2u: not digits:")+s);
	}

	return u;
}

unsigned wbl::s2u(const string &s,unsigned defaultvalue)
{
	errno = 0;
	unsigned u = strtoul(s.c_str(),NULL,10);
	if(errno != 0) {
		return defaultvalue;
	}

	return u;
}
unsigned long wbl::s2ul(const string &s) throw (wbl::comm_error)
{
	errno = 0;
	unsigned long u = strtoul(s.c_str(),NULL,10);
	if(errno != 0) {
		throw wbl::comm_error(string("wbl::s2ul: not digits:")+s);
	}

	return u;
}

unsigned long wbl::s2ul(const string &s,unsigned long defaultvalue)
{
	errno = 0;
	unsigned long u = strtoul(s.c_str(),NULL,10);
	if(errno != 0) {
		return defaultvalue;
	}

	return u;
}
unsigned wbl::sx2u(const string &s) throw (wbl::comm_error)
{
	errno = 0;
	unsigned u = strtoul(s.c_str(),NULL,16);
	if(errno != 0) {
		throw wbl::comm_error(string("wbl::sx2u: not xdigits:")+s);
	}

	return u;
}

unsigned wbl::sx2u(const string &s,unsigned defaultvalue)
{
	errno = 0;
	unsigned u = strtoul(s.c_str(),NULL,16);
	if(errno != 0) {
		return defaultvalue;
	}

	return u;
}
unsigned long wbl::sx2ul(const string &s) throw (wbl::comm_error)
{
	errno = 0;
	unsigned long u = strtoul(s.c_str(),NULL,16);
	if(errno != 0) {
		throw wbl::comm_error(string("wbl::sx2u: not xdigits:")+s);
	}

	return u;
}

unsigned long wbl::sx2ul(const string &s,unsigned long defaultvalue)
{
	errno = 0;
	unsigned long u = strtoul(s.c_str(),NULL,16);
	if(errno != 0) {
		return defaultvalue;
	}

	return u;
}

int wbl::s2i(const string &s) throw (wbl::comm_error)
{
	errno = 0;
	int i = strtol(s.c_str(),NULL,10);
	if(errno != 0) {
		throw wbl::comm_error(string("wbl::s2i: not xdigits:")+s);
	}

	return i;
}

int wbl::s2i(const string &s,int defaultvalue) 
{
	errno = 0;
	int i = strtol(s.c_str(),NULL,10);
	if(errno != 0) {
		return defaultvalue;
	}

	return i;
}
long wbl::s2l(const string &s) throw (wbl::comm_error)
{
	errno = 0;
	long i = strtol(s.c_str(),NULL,10);
	if(errno != 0) {
		throw wbl::comm_error(string("wbl::s2i: not xdigits:")+s);
	}

	return i;
}

long wbl::s2l(const string &s,long defaultvalue) 
{
	errno = 0;
	long i = strtol(s.c_str(),NULL,10);
	if(errno != 0) {
		return defaultvalue;
	}

	return i;
}
char wbl::s2c(const string &s)
{
	if(s.length()==0) return 0;
	return s.c_str()[0];
}

char wbl::s2c(const string &s,char defaultvalue)
{
	if(s.length()==0) return defaultvalue;
	return s.c_str()[0];
}

size_t wbl::s2b(const string &s,char *b,size_t maxlen) throw (wbl::comm_error)
{
	if(maxlen==0) return 0;
	if(s.length()==0) return 0;
	size_t i=0;
	char *p=const_cast<char *>(s.c_str());

	while(*p != '\0') {
		if(*p == '%') p++;
		unsigned char digit;
		if(*p >= '0' && *p <= '9') {
			digit = *p - '0';
		} else if(*p >= 'a' && *p <= 'f') {
			digit = *p - 'a' + 0xa;
		} else if(*p >= 'A' && *p <= 'F') {
			digit = *p - 'A' + 0xa;
		} else {
			throw wbl::comm_error(string("wbl::s2b: format error:")+s);
		}
		digit *= 0x10; p++;
		if(*p >= '0' && *p <= '9') {
			digit += *p - '0';
		} else if(*p >= 'a' && *p <= 'f') {
			digit += *p - 'a' + 0xa;
		} else if(*p >= 'A' && *p <= 'F') {
			digit += *p - 'A' + 0xa;
		} else {
			throw wbl::comm_error(string("wbl::s2b: format error:")+s);
		}
		b[i++] = digit; p++;

		if(i==maxlen) break;
		if(*p == ' ') p++;
	}
	return i;
}

std::string wbl::b2s(const char * b, size_t len, unsigned lf)
{
	string s;
	char sTmp[1024];
	unsigned idx = 0;
	if(len == 0) return s;
	if(lf == 0) {
		for(unsigned i=0;i<len;i++) {
			if(i>0) sTmp[idx++] = ' ';
			sTmp[idx++] = c_b2s[(unsigned char)b[i]][0];
			sTmp[idx++] = c_b2s[(unsigned char)b[i]][1];
			if(idx >= 1020) {
				sTmp[idx] = '\0';
				s += sTmp;
				idx = 0;
			}
		}
		if(idx > 0) {
			sTmp[idx] = '\0';
			s += sTmp;
		}
	} else {
		for(unsigned i=0;i<len;i++) {
			if(i>0) {
				if(i%(lf*2) == 0) sTmp[idx++] = '\n';
				else if(i%lf == 0) {sTmp[idx++] = ' ';sTmp[idx++] = ' ';}
				else sTmp[idx++] = ' ';
			} 
			sTmp[idx++] = c_b2s[(unsigned char)b[i]][0];
			sTmp[idx++] = c_b2s[(unsigned char)b[i]][1];
			if(idx >= 1020) {
				sTmp[idx] = '\0';
				s += sTmp;
				idx = 0;
			}
		}
		if(idx > 0) {
			sTmp[idx] = '\0';
			s += sTmp;
		}
	}

	return s;
}

time_t wbl::s2t(const string &s,const string& format) throw (comm_error)
{
	struct tm curr;

	if(strptime(s.c_str(),format.c_str(),&curr) == NULL) {
		throw comm_error(string("s2t: format error:")+s+","+format);
	}
	return mktime(&curr);
}

string wbl::trim_left(const string &s,const string& filt)
{
	char *head=const_cast<char *>(s.c_str());
	char *p=head;
	while(*p) {
		bool b = false;
		for(size_t i=0;i<filt.length();i++) {
			if((unsigned char)*p == (unsigned char)filt.c_str()[i]) {b=true;break;}
		}
		if(!b) break;
		p++;
	}
	return string(p,0,s.length()-(p-head));
}

string wbl::trim_right(const string &s,const string& filt)
{
	if(s.length() == 0) return string();
	char *head=const_cast<char *>(s.c_str());
	char *p=head+s.length()-1;
	while(p>=head) {
		bool b = false;
		for(size_t i=0;i<filt.length();i++) {
			if((unsigned char)*p == (unsigned char)filt.c_str()[i]) {b=true;break;}
		}
		if(!b) {break;}
		p--;
	}
	return string(head,0,p+1-head);
}

string wbl::trim_head(string& s,const string& split)
{
	string head,body;

	char *phead=const_cast<char *>(s.c_str());
	char *p=phead;
	while(*p!=0) {
		bool match = false;
		char *p1=const_cast<char *>(split.c_str());
		while(*p1!=0) {
			if((unsigned char)*p1 == (unsigned char)*p) {
				match = true;
				break;
			}
			p1++;
		}
		if(match) {
			head = s.substr(0,p-phead);
			body = trim_left(s.substr(p-phead),split);
			break;
		}
		p++;
	}
	if(*p == 0) head = s;
	s = body;
	return head;
}


