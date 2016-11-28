
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



#ifndef __WBL_CONCURRENT_QUEUE_H__
#define __WBL_CONCURRENT_QUEUE_H__

#include <memory>
#include "atomicity.h"

#include "concurrent_node_factory.h"

namespace wbl
{
namespace concurrent_detail
{
	// 由于对node.next的写入仅当next为NULL时可能发生(push中)，因此为节省空间，可以去掉ver，用非法的指针值当ver用
	// 由于字节对齐的缘故，node指针必定不会指向奇数地址值，因此一奇数地址皆为非法指针。
	// 每一次设为NULL都替代为设成一个"唯一的"(在相当长时间内)的非法指针，以达到版本号的效果。
	// 当指针值不匹配时，修改将不成功，以保证修改的原子性
	// 从逻辑上，这些奇数指针都被看作NULL处理
	inline bool is_valid_node_ptr(void * p)
	{
		return !(((long) p) & 0x01);
	}
	extern long _invalid_ptr;
	inline void * invalid_node_ptr()
	{
		return (void *) atomicity::fetch_and_add(&_invalid_ptr, (long) 2);
	}

	struct node_list
	{
		volatile ptr_wrap		head __attribute__ ((aligned (16)));
		volatile ptr_wrap		tail __attribute__ ((aligned (16)));

		void init(node * init_node);
		node * pop();
		void push(node * n);
		void push_all(node * first, node * last, size_t count);
		bool peek(void * p, size_t len);
		size_t size();
		node * pop(void * p, size_t len);
/*
		template < typename T >
		node * pop(T& v)
		{
			for (; ;) {
				ptr_wrap h = head;
				ptr_wrap t = tail;
				node * first = h.ptr->next;
				if (h == head) {
					if (h.ptr == t.ptr) {
						if (!is_valid_node_ptr(first))
							return NULL;
						tail.cas(t, first);
					} else if (is_valid_node_ptr(first)) {	// 当h和t赋值后，node重用一圈导致h.ptr == head.ptr时，有可能出现first为NULL切h.ptr == t.ptr的情况
						v = first->get_item<T>();	// 由于节点需要重用，因此赋值操作只能放在head.cas之前
						if (head.cas(h, first))
							return h.ptr;
					}
				}
			}
		}
		*/
	};
}

template < typename T >
class concurrent_queue : private concurrent_node_factory<T>
{
	typedef concurrent_queue<T>						this_t;
	typedef concurrent_node_factory<T>				base_t;
	using base_t::create_node;
	using base_t::destroy_node;
public:
	typedef size_t			 						size_type;
	typedef T										value_type;
	typedef value_type	*							pointer;
	typedef const value_type *						const_pointer;
	typedef value_type&								reference;
	typedef const value_type&						const_reference;
private:
	typedef concurrent_detail::node					node;
	typedef concurrent_detail::node_list			node_list;

	mutable node_list			_q;		/// 队列元素

	concurrent_queue(const concurrent_queue&);
	concurrent_queue& operator=(const concurrent_queue&);
public:
	concurrent_queue()
	{
		node * p = create_node(value_type());
		_q.init(p);
	}

	~concurrent_queue()
	{
		while (node * p = _q.pop())
			destroy_node(p);
	}

	void push(const value_type& v)
	{
		node * p = create_node(v);
		_q.push(p);
	}

	template < typename RI >
	void push_all(RI first, RI last)
	{
		if (first == last)
			return;
		node * px = create_node(*first);
		node * p = px;
		size_type n = 1;
		for (++first; first != last; ++first, ++n) {
			p->next = create_node(*first);
			p = p->next;
		}
		_q.push_all(px, p, n);
	}

	bool pop(value_type& v)
	{
		//node * p = _q.pop(v);
		node * p = _q.pop(&v, sizeof(v));
		if (p == NULL)
			return false;
		destroy_node(p);
		return true;
	}

	bool peek(value_type& v)
	{
		return _q.peek(&v, sizeof(v));
		//return _q.peek(v);
	}

	bool empty() const
	{
		return size() == 0;
	}

	size_type size() const
	{
		return _q.size();
	}

	void clear()
	{
		while (node * p = _q.pop())
			destroy_node(p);
	}

	template < typename Op >
	void clear(Op op)
	{
		value_type v;
		while(pop(v))
			op(v);
	}
};
}

#endif

