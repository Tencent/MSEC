
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



/*
 */

#include <iomanip>
#include <sys/time.h>
#include <iostream>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

#include "thread_pool.h"

using namespace wbl;
using namespace std;

class Tester {
public:
	Tester(){}
	~Tester() {}
	void run();
};

struct Foo : public runnable
{
	void run()
	{
		usleep(10);
		printf("%ld %s\n", pthread_self(), "abc");
	}
};

struct Foo2
{
	int test()
	{
		usleep(10);
		cout << "Foo2::test" << endl;
		return 0;
	}

	void test() const
	{
		usleep(10);
		cout << "Foo2::test const" << endl;
	}

	void test_ref2(int& n, const std::string& a) const
	{
		usleep(10);
		cout << "Foo2::test_ref2" << "\t&n: " << n << "\ta: " << a << endl;
	}

	void test_const() const
	{
		cout << "test_const" << endl;
	}

	int test_ref(int& n)
	{
		n = 100;
		cout << "in Foo2::test_ref: " << &n << endl;
		return 0;
	}
};

void test_fun()
{
	cout << "test_fun" << endl;
}

int test_cref(const std::string& s)
{
	cout << "in test_cref: " << &s << endl;
	return 0;
}

class B
{
public:
	virtual ~B(){}
	virtual void f()
	{
		cout << "B::f" << endl;
	}
};

/** 线程数据
 */
class Data : public thread_data
{
public:
	pthread_t n;
};

class D : public B
{
public:
	virtual void f()
	{
		Data * p = thread_pool::get_thread_data<Data>();	// 取得线程的私有数据
		assert(pthread_self() == p->n);		// init()里已初始化n为当前线程id，因此该等式成立
		cout << "D::f(), thread id: " << pthread_self() << "\tthread data: " << p->n << endl;
	}
};

void init()
{
	Data * d = new Data();
	d->n = pthread_self();
	thread_pool::set_thread_data(d, true); // 设置线程私有数据，第二个参数true表示线城池负责该数据的释放
}

void Tester::run()
{
	D d;
	B& b = d;
	thread_pool tp(100, init);		// 建立100个线程的线城池，并对每个线程调用init()作初始化
	tp.execute_memfun(d, &D::f);	// 虚函数，此处会调用到D::f
	tp.execute_memfun(b, &B::f);	// 虚函数，此处会调用到正确的派生类的虚函数，D::f
	tp.execute_memfun(d, &B::f);	// 虚函数，同上
	int n = 0;
	std::string s;
	Foo2 foo2;
	const Foo2& cfoo2 = foo2;
	for(int i = 0; i < 100; ++i){
		tp.execute(new Foo());								// 执行一个runnable
		tp.execute_fun(&test_fun);							// 执行一个普通函数
		tp.execute_memfun(cfoo2, &Foo2::test_const);		// const 成员函数
		tp.execute_memfun(foo2, &Foo2::test_ref2, ref(n), s);	// 带参成员函数
		tp.execute_memfun(foo2, &Foo2::test);				// 成员函数
		tp.execute_memfun(cfoo2, &Foo2::test);				// const成员函数（函数名与上同，可根据传入的第一个参数自动识别）
		tp.execute_memfun(foo2, &Foo2::test_ref, ref(n));	// 传引用参数时必须加上ref，否则编译不过
		tp.execute_fun(&test_cref, ref(s));					// 传常量引用时若不加ref，实际传的是值
	}
	cout << tp << endl;	// 输出该线程池的信息
	usleep(5000);
	cout << tp << endl;
	tp.wait();	// 等待所有任务执行完成
	cout << tp << endl;
}

void fff()
{
	cout << "fff" << endl;
}

int main(int argc, char * argv[]) 
{
	Tester test;
	test.run();
	return 0;
}



