
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


#ifndef __TCE_THREAD_H__
#define __TCE_THREAD_H__

#if defined(WIN32)
#include <process.h>
#elif defined(_POSIX_THREADS)
#include <pthread.h>
#endif

#include "tce_singleton.h"
#include "tce_utils.h"

namespace tce {
	template <typename FUNC>
	class CThread : private CNonCopyAble {
		typedef CThread this_type;
	public:
		CThread(FUNC func) : Id_(0), Func_(func), End_(true) {
		}

		~CThread() {
			Stop();
		}

		bool Start(void * param) {
			End_ = false;
			Param_.This = this;
			Param_.Param = param;
#if defined(WIN32)
			uint32_t tid;
			Id_ = (uint32_t)_beginthreadex(0, 0, ThreadProc, &Param_, 0, &tid);
			if (Id_ == -1) Id_ = 0;
#elif defined(_POSIX_THREADS)
			if (pthread_create(&Id_, 0, ThreadProc, &Param_)) {
				Id_ = 0;
			}
#endif
			if (Id_ == 0) {
				End_ = true;
				return false;
			}
			else {
				return true;
			}
		}

		void Stop() {
			if (0 != Id_) {
				while(!End_) {
					xsleep(10);
				}
				Id_ = 0;
			}
		}

	private:
#if defined(WIN32)
		static uint32_t __stdcall ThreadProc(void * param)
#elif defined(_POSIX_THREADS)
		static void* ThreadProc(void* param)
#endif
		{
			THEAD_PARAM * p = static_cast<THEAD_PARAM *>(param);
			this_type * This = static_cast<this_type *>(p->This);
			This->Func_(p->Param);
			This->End_ = true;
			return 0;
		}

	private:
#if defined(WIN32)
		uint32_t Id_;
#elif defined(_POSIX_THREADS)
		pthread_t Id_;
#endif
		struct THEAD_PARAM
		{
			void * This;
			void * Param;
		};
		THEAD_PARAM Param_;
		FUNC Func_;
		volatile bool End_;
	}; //end class CThread;
}; //end namespace fund;
#endif
