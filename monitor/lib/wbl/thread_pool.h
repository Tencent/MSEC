
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
//     线程池
//
//////////////////////////////////////////////////////////////////////////

#ifndef __WBL_THREAD_POOL_H__
#define __WBL_THREAD_POOL_H__

#include <vector>
#include <iosfwd>
#include <memory>
#include <semaphore.h>
#include <pthread.h>
#include "concurrent_queue.h"
#include "executor.h"

namespace wbl
{

//Semophore
class Semaphore
{
	sem_t	m_sem;

	Semaphore(const Semaphore&);
	Semaphore& operator=(const Semaphore&);
public:
	explicit Semaphore(int pshared = 0, unsigned int value = 0)	{ ::sem_init(&m_sem, pshared, value); }
	~Semaphore()								{ ::sem_destroy(&m_sem); }

	bool Post()									{ return 0 == ::sem_post(&m_sem); }
	void Wait()									{ ::sem_wait(&m_sem); }
#if _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600
	bool Wait(const struct timespec& abstime)	{ return 0 == ::sem_timedwait(&m_sem, &abstime); }
#endif
	bool TryWait()								{ return 0 == ::sem_trywait(&m_sem); }
	int GetValue()
	{
		int val = 0;
		::sem_getvalue(&m_sem, &val);
		return val;
	}
};
 
//Barrier
class Barrier
{
private:
	pthread_barrier_t m_sect;
	
	Barrier(const Barrier&);
	Barrier& operator=(const Barrier&);
public:
	Barrier(unsigned int count)	{ ::pthread_barrier_init(&m_sect, NULL, count); }
	~Barrier()						{ ::pthread_barrier_destroy(&m_sect); }
	int Wait()						{ return ::pthread_barrier_wait(&m_sect); }
};

/** delete operation
 */
struct checked_delete
{
	template < typename T >
	void operator()(T *p) const
	{
		try{
			delete p;
		}catch (...) {
		}
	}
};

/** thread data
 */
struct thread_data
{
	virtual ~thread_data() {}
};

namespace thread_pool_detail
{
	enum { eRunning, eWaiting, eStoped };

	struct thread_info
	{
		pthread_t		id;			///< thread id
		volatile int	status;		///< thread status, one of eRunning, eWaiting, eStoped
		thread_data *	data;		///< thread private data
		bool			own_data;	///< own the thread data?

		thread_info()
			: id(0), status(eRunning), data(0), own_data(false)
		{}
	};

	extern "C" void create_thread_key();
	//extern "C" void create_priority_thread_key();
	extern "C" void * thread_pool_proc(void* pPara);
	extern "C" void * priority_thread_pool_proc(void* pPara);
}

/** thread pool
 */
class thread_pool : public wbl::executor
{
	typedef thread_pool_detail::thread_info	thread_info;
	//typedef blocking_queue<runnable *>		QueueT;
	typedef concurrent_queue<runnable *>		QueueT;

	typedef std::vector<thread_info>::iterator	VIT;
	typedef std::vector<thread_info>::const_iterator	CVIT;

	QueueT						_queue;		///< task queue
	Semaphore					_sem;		///< for queue block
	std::vector<thread_info>	_tid;		///< thread info
	volatile bool				_stop_all;	///< stop all thread flag
	runnable *					_init;		///< thread initializer
	static pthread_key_t		_tkey;		///< thread key
	Barrier *					_barrier;	///< barrier for wait

	thread_pool(const thread_pool&);
	thread_pool& operator=(const thread_pool&);
public:
	typedef std::vector<thread_info>::size_type	size_type;

	/** constructor
	 *  \param thread_count	thread count
	 *  \param initOp	initializer
	 */
	template < typename Op >
	thread_pool(int thread_count, Op initOp)
		: _stop_all(false)
		, _init(NULL)
		, _barrier(NULL)
	{
		start(thread_count, initOp);
	}

	/** constructor
	 *  \param thread_count	thread count
	 */
	explicit thread_pool(int thread_count);

	thread_pool();
	~thread_pool();

	/** set thread's private data
	 *  \param data		new thread data
	 *  \param own		own the data
	 *  \return			the old thread data
	 */
	static void set_thread_data(thread_data * data, bool own = false);

	/// get thread's private data
	static thread_data * get_thread_data();

	/// get thread's private data
	template < typename T >
	static T * get_thread_data()
	{
		return static_cast<T*>(get_thread_data());
	}

	/** start the thread pool
	 *  \param thread_count	thread count
	 *  \param initOp	initializer
	 *  \param dummy	useless parameter, you should ignore it; only use for overload function
	 *  \return success?
	 */
	template < typename Op >
	bool start(int thread_count, Op initOp, typename disable_if<is_convertible<Op, runnable*> >::type* dummy = 0)
	{
		return start_impl(thread_count, make_fun_runnable(initOp));
	}

	/** start the thread pool
	 *  \param thread_count	thread count
	 *  \param initOp	initializer
	 *  \return success?
	 */
	bool start(int thread_count, runnable * initOp)
	{
		return start_impl(thread_count, initOp);
	}

	/** start the thread pool
	 *  \param thread_count	thread count
	 *  \return success?
	 */
	bool start(int thread_count)
	{
		return start_impl(thread_count);
	}

	/// get thread count of the thread pool
	size_type get_thread_count() const
	{
		return _tid.size();
	}

	/// get active thread count
	size_type get_active_thread_count() const
	{
		return count(thread_pool_detail::eRunning);
	}

	/// get waiting queue's length
	size_type get_queue_length() const
	{
		return _queue.size();
	}

	/// execute a task
	void execute(runnable * r)
	{
		//_queue.push_b(r);
		_queue.push(r);
		_sem.Post();
	}

	/// stop all thread
	void stop(bool force = false);

	/// wait for all task run finish
	void wait();

	/** wait for all task run finish unit timeout
	 *  \param timeout		timeout in milliseconds
	 *  \return				all task finish?
	 */
	bool wait(int timeout);
private:
	bool start_impl(int thread_count, runnable * init = NULL);
	size_type count(int status) const;
	void on_thread_init(thread_info * ti);
	friend void * thread_pool_detail::thread_pool_proc(void* pPara);
	friend void thread_pool_detail::create_thread_key();
	friend class priority_thread_pool;
};

std::ostream& operator<<(std::ostream& ost, const thread_pool& tp);

}

#endif

