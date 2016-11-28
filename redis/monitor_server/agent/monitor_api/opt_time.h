
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


// opt_time_r.h
//
// thread safe version of opt_time.h
//
//

#ifndef __OPT_TIME_H__
#define __OPT_TIME_H__

#include <stdint.h>
#include <sys/time.h>
#include <string.h>

//
// optimized gettimeofday()/time()
//
// Limitations:
//  1, here we assume the CPU speed is 1GB, if your CPU is 4GB, it will run well, but if your CPU is 10GB, please adjust CPU_SPEED_GB
//  2, these functions have precision of 1ms, if you wish higher precision, please adjust REGET_TIME_US, but it will degrade performance
//

#ifdef __x86_64__
#define RDTSC() ({ register uint32_t a,d; __asm__ __volatile__( "rdtsc" : "=a"(a), "=d"(d)); (((uint64_t)a)+(((uint64_t)d)<<32)); })
#else
#define RDTSC() ({ register uint64_t tim; __asm__ __volatile__( "rdtsc" : "=A"(tim)); tim; })
#endif

// atomic operations
#ifdef __x86_64__
    #define ASUFFIX "q"
#else
    #define ASUFFIX "l"
#endif
#define XCHG(ptr, val) __asm__ __volatile__("xchg"ASUFFIX" %2,%0" :"+m"(*ptr), "=r"(val) :"1"(val))
#define AADD(ptr, val) __asm__ __volatile__("lock ; add"ASUFFIX" %1,%0" :"+m" (*ptr) :"ir" (val))
#define CAS(ptr, val_old, val_new)({ char ret; __asm__ __volatile__("lock; cmpxchg"ASUFFIX" %2,%0; setz %1": "+m"(*ptr), "=q"(ret): "r"(val_new),"a"(val_old): "memory"); ret;})


#define REGET_TIME_US_GTOD   1
#define REGET_TIME_US_TIME   1
#define CPU_SPEED_GB    1  // assume a 1GB CPU

static inline int opt_gettimeofday(struct timeval *tv, __timezone_ptr_t not_used)
{
	static volatile uint64_t walltick;
	static volatile struct timeval walltime;
	static volatile long lock = 0;
	const unsigned int max_ticks = CPU_SPEED_GB*1000*REGET_TIME_US_GTOD;
	
	if(walltime.tv_sec==0 || (RDTSC()-walltick) > max_ticks)
	{
		if(lock==0 && CAS(&lock, 0UL, 1UL)) // try lock
		{
			gettimeofday((struct timeval*)&walltime, not_used);
			walltick = RDTSC();
			lock = 0; // unlock
		}
		else // try lock failed, use system time
		{
			return gettimeofday(tv, not_used);
		}
	}
	memcpy(tv, (void*)&walltime, sizeof(struct timeval));
	return 0;
}

// same algorithm with gettimeofday, except with different precision defined as REGET_TIME_US_TIME
static inline time_t opt_time(time_t *t)
{
	static volatile uint64_t walltick;
	static volatile struct timeval walltime;
	static volatile long lock = 0;
	const unsigned int max_ticks = CPU_SPEED_GB*1000*REGET_TIME_US_TIME;
	
	if(walltime.tv_sec==0 || (RDTSC()-walltick) > max_ticks)
	{
		if(lock==0 && CAS(&lock, 0UL, 1UL)) // try lock
		{
			gettimeofday((struct timeval*)&walltime, NULL);
			walltick = RDTSC();
			lock = 0; // unlock
		}
		else // try lock failed, use system time
		{
			struct timeval tv;
			gettimeofday(&tv, NULL);
			if(t) *t = tv.tv_sec;
			return tv.tv_sec;
		}
	}
	if(t) *t = walltime.tv_sec;
	return walltime.tv_sec;
}

#ifndef gettimeofday
#define gettimeofday(a, b) opt_gettimeofday(a, b)
#endif
#ifndef time
#define time(t) opt_time(t)
#endif

//
// Optimized Attr_API based on opt_time()
// Add to a static counter and report once in a second
// Limitations:
//   1, attr should be a const value, you must not call Frequent_Attr_API(iAttr, 1) when iAttr differs for each call
//   2, should only be used for frequent reports rather than exception cases, since Frequent_Attr_API does not call Attr_API at once
//

// If you don't wish to call opt_time() frequently, supply your own current time
#define Frequent_Attr_API_Ext(attr, count, curtime) do { \
	static volatile time_t tPrevReport##attr = 0; \
	static volatile long iReportNum##attr = 0; \
	AADD(&iReportNum##attr, count); \
	if(tPrevReport##attr != curtime) \
	{ \
		long cnt = 0; \
		XCHG(&iReportNum##attr, cnt);\
		if(cnt) \
		{ \
			Attr_API(attr, cnt); \
		} \
		tPrevReport##attr = curtime; \
	} \
} while(0)

#define Frequent_Attr_API(attr, count) do { time_t t=opt_time(NULL); Frequent_Attr_API_Ext(attr, count, t); } while(0)


#ifdef TEST_TIME

#include <stdio.h>

int main(int argc, char *argv[])
{
	if(argc!=2)
	{
		printf("usage:\n");
		printf("       %s time           # test time performance\n", argv[0]);
		printf("       %s gettimeofday   # test gettimeofday performance\n", argv[0]);
		printf("       %s attr           # test attr api performance\n", argv[0]);
		return -1;
	}
	if(argv[1][0]=='g')
	{
		int i;
		struct timeval tv;
		for(i=0; i<100000000; i++)
		{
			opt_gettimeofday(&tv, NULL);
			if(i%10000000==0)
				printf("%u-%u\n", tv.tv_sec, tv.tv_usec);
		}
	}
	else if(argv[1][0]=='t')
	{
		int i;
		time_t t;
		for(i=0; i<100000000; i++)
		{
			t = opt_time(NULL);
			if(i%10000000==0)
				printf("%u\n", t);
		}
	}
	else
	{
		int i;
		for(i=0; i<10000000; i++)
		{
			Frequent_Attr_API(51830, 1);
			Frequent_Attr_API(51831, 1);
			Frequent_Attr_API(51832, 1);
			Frequent_Attr_API(51833, 1);
			Frequent_Attr_API(51834, 1);
			Frequent_Attr_API(51835, 1);
			Frequent_Attr_API(51836, 1);
			Frequent_Attr_API(51837, 1);
			Frequent_Attr_API(51838, 1);
			Frequent_Attr_API(51839, 1);
		}
	}
}

#endif


#endif //__OPT_TIME_H__

