
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
//
//////////////////////////////////////////////////////////////////////////

#ifndef __WBL_EXECUTOR_H__
#define __WBL_EXECUTOR_H__

#include "runnable.h"

namespace wbl
{

class executor
{
protected:
	executor() {}
public:
	virtual ~executor() {}

	virtual void execute(runnable * r) = 0;

	template < typename FUN >
	void execute_fun(FUN fun)
	{
		execute(make_fun_runnable(fun));
	}

	template < typename FUN, typename PARA >
	void execute_fun(FUN fun, PARA para)
	{
		execute(make_fun_runnable(fun, para));
	}

	template < typename FUN, typename PARA1, typename PARA2 >
	void execute_fun(FUN fun, PARA1 para1, PARA2 para2)
	{
		execute(make_fun_runnable(fun, para1, para2));
	}

	template < typename FUN, typename PARA1, typename PARA2, typename PARA3 >
	void execute_fun(FUN fun, PARA1 para1, PARA2 para2, PARA3 para3)
	{
		execute(make_fun_runnable(fun, para1, para2, para3));
	}

	template < typename FUN, typename PARA1, typename PARA2, typename PARA3, typename PARA4 >
	void execute_fun(FUN fun, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4)
	{
		execute(make_fun_runnable(fun, para1, para2, para3, para4));
	}

	template < typename FUN, typename PARA1, typename PARA2, typename PARA3, typename PARA4, typename PARA5 >
	void execute_fun(FUN fun, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, PARA5 para5)
	{
		execute(make_fun_runnable(fun, para1, para2, para3, para4, para5));
	}

	template < typename FUN, typename PARA1, typename PARA2, typename PARA3, typename PARA4, typename PARA5, typename PARA6 >
	void execute_fun(FUN fun, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, PARA5 para5, PARA6 para6)
	{
		execute(make_fun_runnable(fun, para1, para2, para3, para4, para5, para6));
	}

	template < typename T, typename T2, typename RT >
	void execute_memfun(T& obj, RT (T2::* mf)(), typename memfun_disable_if<T, T2, false>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf));
	}

	template < typename T, typename T2, typename RT >
	void execute_memfun(const T& obj, RT (T2::* mf)() const, typename memfun_disable_if<const T, T2, true>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf));
	}

	template < typename T, typename T2, typename RT, typename P, typename PARA >
	void execute_memfun(T& obj, RT (T2::* mf)(P), PARA para, typename memfun_disable_if<T, T2, false, P, PARA>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para));
	}

	template < typename T, typename T2, typename RT, typename P, typename PARA >
	void execute_memfun(const T& obj, RT (T2::* mf)(P) const, PARA para, typename memfun_disable_if<const T, T2, true, P, PARA>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename PARA1, typename PARA2 >
	void execute_memfun(T& obj, RT (T2::* mf)(P1, P2), PARA1 para1, PARA2 para2, typename memfun_disable_if<T, T2, false, P1, PARA1, P2, PARA2>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para1, para2));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename PARA1, typename PARA2 >
	void execute_memfun(const T& obj, RT (T2::* mf)(P1, P2) const, PARA1 para1, PARA2 para2, typename memfun_disable_if<const T, T2, true, P1, PARA1, P2, PARA2>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para1, para2));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, 
		typename PARA1, typename PARA2, typename PARA3 >
	void execute_memfun(T& obj, RT (T2::* mf)(P1, P2, P3), PARA1 para1, PARA2 para2, PARA3 para3, typename memfun_disable_if<T, T2, false, P1, PARA1, P2, PARA2, P3, PARA3>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para1, para2, para3));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, 
		typename PARA1, typename PARA2, typename PARA3 >
	void execute_memfun(const T& obj, RT (T2::* mf)(P1, P2, P3) const, PARA1 para1, PARA2 para2, PARA3 para3, typename memfun_disable_if<const T, T2, true, P1, PARA1, P2, PARA2, P3, PARA3>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para1, para2, para3));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, typename P4,
		typename PARA1, typename PARA2, typename PARA3, typename PARA4 >
	void execute_memfun(T& obj, RT (T2::* mf)(P1, P2, P3, P4), PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, typename memfun_disable_if<T, T2, false, P1, PARA1, P2, PARA2, P3, PARA3, P4, PARA4>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para1, para2, para3, para4));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, typename P4,
		typename PARA1, typename PARA2, typename PARA3, typename PARA4 >
	void execute_memfun(const T& obj, RT (T2::* mf)(P1, P2, P3, P4) const, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, typename memfun_disable_if<const T, T2, true, P1, PARA1, P2, PARA2, P3, PARA3, P4, PARA4>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para1, para2, para3, para4));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, typename P4, typename P5,
		typename PARA1, typename PARA2, typename PARA3, typename PARA4, typename PARA5 >
	void execute_memfun(T& obj, RT (T2::* mf)(P1, P2, P3, P4, P5), PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, PARA5 para5, typename memfun_disable_if<T, T2, false, P1, PARA1, P2, PARA2, P3, PARA3, P4, PARA4, P5, PARA5>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para1, para2, para3, para4, para5));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, typename P4, typename P5,
		typename PARA1, typename PARA2, typename PARA3, typename PARA4, typename PARA5 >
	void execute_memfun(const T& obj, RT (T2::* mf)(P1, P2, P3, P4, P5) const, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, PARA5 para5, typename memfun_disable_if<const T, T2, true, P1, PARA1, P2, PARA2, P3, PARA3, P4, PARA4, P5, PARA5>::type dummy = 0)
	{
		execute(make_memfun_runnable(obj, mf, para1, para2, para3, para4, para5));
	}
};

class priority_executor 
{
protected:
	priority_executor() {}
public:
	virtual ~priority_executor() {}

	virtual void execute(int priority, runnable * r) = 0;

	template < typename FUN >
	void execute_fun(int priority, FUN fun)
	{
		execute(priority, make_fun_runnable(fun));
	}

	template < typename FUN, typename PARA >
	void execute_fun(int priority, FUN fun, PARA para)
	{
		execute(priority, make_fun_runnable(fun, para));
	}

	template < typename FUN, typename PARA1, typename PARA2 >
	void execute_fun(int priority, FUN fun, PARA1 para1, PARA2 para2)
	{
		execute(priority, make_fun_runnable(fun, para1, para2));
	}

	template < typename FUN, typename PARA1, typename PARA2, typename PARA3 >
	void execute_fun(int priority, FUN fun, PARA1 para1, PARA2 para2, PARA3 para3)
	{
		execute(priority, make_fun_runnable(fun, para1, para2, para3));
	}

	template < typename FUN, typename PARA1, typename PARA2, typename PARA3, typename PARA4 >
	void execute_fun(int priority, FUN fun, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4)
	{
		execute(priority, make_fun_runnable(fun, para1, para2, para3, para4));
	}

	template < typename FUN, typename PARA1, typename PARA2, typename PARA3, typename PARA4, typename PARA5 >
	void execute_fun(int priority, FUN fun, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, PARA5 para5)
	{
		execute(priority, make_fun_runnable(fun, para1, para2, para3, para4, para5));
	}

	template < typename FUN, typename PARA1, typename PARA2, typename PARA3, typename PARA4, typename PARA5, typename PARA6 >
	void execute_fun(int priority, FUN fun, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, PARA5 para5, PARA6 para6)
	{
		execute(priority, make_fun_runnable(fun, para1, para2, para3, para4, para5, para6));
	}

	template < typename T, typename T2, typename RT >
	void execute_memfun(int priority, T& obj, RT (T2::* mf)(), typename memfun_disable_if<T, T2, false>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf));
	}

	template < typename T, typename T2, typename RT >
	void execute_memfun(int priority, const T& obj, RT (T2::* mf)() const, typename memfun_disable_if<const T, T2, true>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf));
	}

	template < typename T, typename T2, typename RT, typename P, typename PARA >
	void execute_memfun(int priority, T& obj, RT (T2::* mf)(P), PARA para, typename memfun_disable_if<T, T2, false, P, PARA>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para));
	}

	template < typename T, typename T2, typename RT, typename P, typename PARA >
	void execute_memfun(int priority, const T& obj, RT (T2::* mf)(P) const, PARA para, typename memfun_disable_if<const T, T2, true, P, PARA>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename PARA1, typename PARA2 >
	void execute_memfun(int priority, T& obj, RT (T2::* mf)(P1, P2), PARA1 para1, PARA2 para2, typename memfun_disable_if<T, T2, false, P1, PARA1, P2, PARA2>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para1, para2));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename PARA1, typename PARA2 >
	void execute_memfun(int priority, const T& obj, RT (T2::* mf)(P1, P2) const, PARA1 para1, PARA2 para2, typename memfun_disable_if<const T, T2, true, P1, PARA1, P2, PARA2>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para1, para2));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, 
		typename PARA1, typename PARA2, typename PARA3 >
	void execute_memfun(int priority, T& obj, RT (T2::* mf)(P1, P2, P3), PARA1 para1, PARA2 para2, PARA3 para3, typename memfun_disable_if<T, T2, false, P1, PARA1, P2, PARA2, P3, PARA3>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para1, para2, para3));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, 
		typename PARA1, typename PARA2, typename PARA3 >
	void execute_memfun(int priority, const T& obj, RT (T2::* mf)(P1, P2, P3) const, PARA1 para1, PARA2 para2, PARA3 para3, typename memfun_disable_if<const T, T2, true, P1, PARA1, P2, PARA2, P3, PARA3>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para1, para2, para3));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, typename P4,
		typename PARA1, typename PARA2, typename PARA3, typename PARA4 >
	void execute_memfun(int priority, T& obj, RT (T2::* mf)(P1, P2, P3, P4), PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, typename memfun_disable_if<T, T2, false, P1, PARA1, P2, PARA2, P3, PARA3, P4, PARA4>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para1, para2, para3, para4));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, typename P4,
		typename PARA1, typename PARA2, typename PARA3, typename PARA4 >
	void execute_memfun(int priority, const T& obj, RT (T2::* mf)(P1, P2, P3, P4) const, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, typename memfun_disable_if<const T, T2, true, P1, PARA1, P2, PARA2, P3, PARA3, P4, PARA4>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para1, para2, para3, para4));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, typename P4, typename P5,
		typename PARA1, typename PARA2, typename PARA3, typename PARA4, typename PARA5 >
	void execute_memfun(int priority, T& obj, RT (T2::* mf)(P1, P2, P3, P4, P5), PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, PARA5 para5, typename memfun_disable_if<T, T2, false, P1, PARA1, P2, PARA2, P3, PARA3, P4, PARA4, P5, PARA5>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para1, para2, para3, para4, para5));
	}

	template < typename T, typename T2, typename RT, typename P1, typename P2, typename P3, typename P4, typename P5,
		typename PARA1, typename PARA2, typename PARA3, typename PARA4, typename PARA5 >
	void execute_memfun(int priority, const T& obj, RT (T2::* mf)(P1, P2, P3, P4, P5) const, PARA1 para1, PARA2 para2, PARA3 para3, PARA4 para4, PARA5 para5, typename memfun_disable_if<const T, T2, true, P1, PARA1, P2, PARA2, P3, PARA3, P4, PARA4, P5, PARA5>::type dummy = 0)
	{
		execute(priority, make_memfun_runnable(obj, mf, para1, para2, para3, para4, para5));
	}
};

}

#endif

