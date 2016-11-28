
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
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>

#include "thread_pool.h"

namespace wbl
{
using namespace thread_pool_detail;

pthread_key_t	thread_pool::_tkey;

thread_pool::thread_pool(int thread_count)
	: _stop_all(false)
	, _init(NULL)
	, _barrier(NULL)
{
	start(thread_count);
}

thread_pool::thread_pool()
	: _stop_all(false)
	, _init(NULL)
	, _barrier(NULL)
{}

thread_pool::~thread_pool()
{
	stop();
}
/*
namespace
{
	class set_state_op
	{
		volatile int *	_status;
		int				_val;
	public:
		set_state_op(volatile int * status, int val)
			: _status(status)
			  , _val(val)
		{}

		template < typename T >
		bool operator()(const T&)
		{
			*_status = _val;
			return true;
		}
	};
}
*/
namespace thread_pool_detail
{
extern "C"
{
	void free_thread_key(void * data)
	{
		thread_info * ti = static_cast<thread_info *>(data);
		if(ti->own_data)
			delete ti->data;
	}

	void create_thread_key()
	{
		::pthread_key_create(&thread_pool::_tkey, free_thread_key);
	}

	void * thread_pool_proc(void* pPara)
	{
		typedef std::pair<thread_info*, thread_pool*>	PairT;
		PairT * p = reinterpret_cast<PairT *>(pPara);
		thread_pool * tp = p->second;
		thread_info * ti = p->first;
		delete p;
		tp->on_thread_init(ti);
		thread_pool::QueueT& bq = tp->_queue;
		//set_state_op sop(&ti->status, eRunning);
		while(!tp->_stop_all){
			runnable * r = NULL;
			//*
			ti->status = eWaiting;
			if (bq.pop(r)) {
				std::auto_ptr<runnable> p(r);
				try{
					ti->status = eRunning;
					r->run();
				}catch(...){
				}
			} else
				tp->_sem.Wait();
			/*/
			switch(bq.pop_if(r, sop))
			{
			case ePopIfSucc:
				{
					std::auto_ptr<runnable> p(r);
					try{
						r->run();
					}catch(...){
					}
				}
				break;
			case ePopIfEmpty:
				tp->_sem.Wait();
				break;
			default:
				assert(false);
			}
			//*/
		}
		ti->status = eStoped;

		return NULL;
	}
}
}

void thread_pool::set_thread_data(thread_data * data, bool own)
{
	thread_info * ti = static_cast<thread_info*>(pthread_getspecific(_tkey));
	if(ti->own_data)
		delete ti->data;
	ti->data = data;
	ti->own_data = own;
}

thread_data * thread_pool::get_thread_data()
{
	thread_info * ti = static_cast<thread_info*>(pthread_getspecific(_tkey));
	return ti->data;
}

bool thread_pool::start_impl(int thread_count, runnable * init)
{
	if(_tid.size() > 0)
		return false;
	_init = init;

	static pthread_once_t once = PTHREAD_ONCE_INIT;
	::pthread_once(&once, create_thread_key);

	bool result = true;
	typedef std::pair<thread_info*, thread_pool*>	PairT;
	_stop_all = false;
	_tid.resize(thread_count);
	for(VIT i = _tid.begin(); i != _tid.end(); ++i){
		PairT * p = new PairT(&*i, this);
		if(::pthread_create(&i->id, 0, &thread_pool_proc, p) != 0){
			_tid.resize(i - _tid.begin());
			result = false;
			delete p;
			break;
		}
	}
	_barrier = new Barrier(_tid.size() + 1);
	return result;
}

void thread_pool::stop(bool force)
{
	_queue.clear(checked_delete());
	_stop_all = true;
	for (VIT i = _tid.begin(); i != _tid.end(); ++i)
		_sem.Post();
	for (VIT i = _tid.begin(); i != _tid.end(); ++i) {
		pthread_t tid = i->id;
		if(force)
			::pthread_cancel(tid);
		else
			::pthread_join(tid, NULL);
		::pthread_detach(tid);
		i->status = eStoped;
	}
	_tid.clear();
	delete _init;
	_init = NULL;
	delete _barrier;
	_barrier = NULL;
}

void thread_pool::on_thread_init(thread_info * ti)
{
	::pthread_setspecific(_tkey, ti);
	if(_init)
		_init->run();
}

thread_pool::size_type thread_pool::count(int status) const
{
	size_type c = 0;
	for(CVIT i = _tid.begin(); i != _tid.end(); ++i){
		if(i->status == status)
			++c;
	}
	return c;
}

void thread_pool::wait()
{
	/*
	while(!_queue.empty() || get_active_thread_count() > 0)
		usleep(10000);
		*/
	for (size_t i = 0; i < _tid.size(); ++i)
		execute_memfun(*_barrier, &Barrier::Wait);
	_barrier->Wait();
}

bool thread_pool::wait(int timeout)
{
	int lc = timeout * 1000 / 10000;
	for(; (!_queue.empty() || get_active_thread_count() > 0) && lc > 0; --lc)
		usleep(10000);
	return lc > 0;
}

std::ostream& operator<<(std::ostream& ost, const thread_pool& tp)
{
	ost << "{thread_count: " << tp.get_thread_count() 
		<< ", active_thread_count: " << tp.get_active_thread_count()
		<< ", queue_length: " << tp.get_queue_length() << "}";
	return ost;
}


}

