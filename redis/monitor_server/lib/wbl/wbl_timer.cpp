
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


#include <unistd.h>
#include "wbl_timer.h"

namespace wbl 
{
using namespace timer_detail;

static const pthread_t INVALID_TID = 0;

timer::timer()
	: _task(NULL)
	, _tid(INVALID_TID)
	, _interval(0)
{}

timer::~timer()
{
	cancel();
}

void timer::cancel()
{
	if(_tid != INVALID_TID){
		//::pthread_cancel(_tid);
		_interval = 0;
		::pthread_join(_tid, NULL);
		::pthread_detach(_tid);
		delete _task;
		_task = NULL;
		_tid = INVALID_TID;
	}
}

bool timer::schedule(runnable * task, size_t interval)
{
	if(task == NULL || interval == 0)
		return false;
	cancel();
	_task = task;
	_interval = interval * 1000;
	::pthread_create(&_tid, 0, &proc, this);
	return true;
}

namespace timer_detail
{
extern "C" void* proc(void* pPara)
{
	timer * p = static_cast<timer *>(pPara);
	runnable * r = p->_task;
	const size_t max_step = 500 * 1000;
	const size_t loopcount = p->_interval / max_step + 1;
	size_t step = p->_interval / loopcount;
	while(true){
		try{
			for(size_t i = 0; i < loopcount; ++i){
				if(p->_interval == 0)
					return NULL;
				usleep(step);
			}
			r->run();
		}catch(...){
		}
	}

	return NULL;
}
}
}

