
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



//////////////////////////////////////////////////////////////////////////
//
//     wbl_timer
//
//////////////////////////////////////////////////////////////////////////

#ifndef __WBL_TIMER_H__
#define __WBL_TIMER_H__

#include <pthread.h>
#include "runnable.h"

namespace wbl
{

namespace timer_detail
{
extern "C" void * proc(void * pPara);
}

/** timer
 */
class timer
{
	runnable *			_task;			///< time task
	pthread_t			_tid;			///< timer thread id
	volatile size_t		_interval;		///< interval in milliseconds before task is to be executed
public:
	timer();
	~timer();

	/// cancel the timer
	void cancel();

	/** schedule
	 *  \param task		the task to schedule
	 *  \param interval	interval in milliseconds before task is to be executed
	 *  \return			return false if task is null or interval is zero.
	 */
	bool schedule(runnable * task, size_t interval);
private:
	//static void * proc(void * pPara);
	friend void * timer_detail::proc(void * pPara);
};

}

#endif

