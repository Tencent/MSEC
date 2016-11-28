
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



#ifndef __WBL_CONCURRENT_NODE_FACTORY_H__
#define __WBL_CONCURRENT_NODE_FACTORY_H__

#include <stdlib.h>
#include <memory>
#include "meta.h"
#include "atomicity.h"

namespace wbl
{
namespace concurrent_detail
{
	template < typename T >
	struct node_impl
	{
		node_impl * volatile	next;
		T						item;
	};

	struct node
	{
		node *	next;
		void *	item_helper;

		template < typename T >
		inline volatile T& get_item()
		{
			return ((node_impl<T> *) this)->item;
		}

		void * get_item_ptr()	{ return &item_helper; }
	};

	struct ptr_wrap
	{
		node * volatile		ptr;	/// 节点指针
		volatile size_t		ver;	/// 版本号，每次指针修改都会同时被修改，用于防止节点重用时cas不起作用

		inline bool cas(const ptr_wrap& old, node * n) volatile
		{
			ptr_wrap nx = {n, old.ver + 1};
			//nx.ptr = n;
			//nx.ver = old.ver + 1;
			return atomicity::compare_and_swap(this, old, nx);
		}

		inline bool cas(const ptr_wrap& op, const ptr_wrap& np) volatile
		{
			return atomicity::compare_and_swap(this, op, np);
		}
	};

	inline bool operator==(const volatile ptr_wrap& l, const volatile ptr_wrap& r)
	{
		return l.ptr == r.ptr && l.ver == r.ver;
	}

	class node_stack
	{
		volatile ptr_wrap	_top __attribute__ ((aligned (16)));

		bool peek(void * p, size_t len);
	public:
		typedef uint32_t		size_type;

		node_stack()
		{
			_top.ptr = NULL;
			_top.ver = 0;
		}

		node * pop();
		void push(node * n);

		template < typename T >
		bool peek(T& v)
		{
			return peek(&v, sizeof(v));
		}

		bool empty() const { return _top.ptr == NULL; }
		size_type size() const;
	};

	void free_all_node(node_stack& s, std::allocator<char>& a, int block_size);

	template < int block_size >
	struct free_stack_wrapper
	{
		node_stack				_free;
		std::allocator<char>	_alloc;

		~free_stack_wrapper()	{ free_all_node(_free, _alloc, block_size); }
	};

	template < int block_size >
	struct concurrent_node_free_stack
	{
		static free_stack_wrapper<block_size>	_w;
	};

	template < int block_size >
	free_stack_wrapper<block_size> concurrent_node_free_stack<block_size>::_w;

	template < typename T >
	struct is_concurrent_node_factory_allow_type
	{
#ifdef WBL_IS_POD
		enum { value = (WBL_IS_POD(T))};
#else
		enum { value = true };
		//enum { value = ((is_pointer_type<T>::value || is_same_type<T, int>::value || is_same_type<T, long>::value || is_same_type<T, unsigned int>::value || is_same_type<T, unsigned long>::value) || is_same_type<T, float>::value || is_same_type<T, double>::value)};
#endif
	};
}

template < typename T >
class concurrent_node_factory
{
	const static int aligned_size = 8;
	const static int block_size = ((sizeof(T) + sizeof(void *) + aligned_size - 1) / aligned_size) * aligned_size;

	typedef concurrent_detail::concurrent_node_free_stack<block_size>	free_stack_t;
	typedef concurrent_detail::node_impl<T>			node_impl;
	typedef concurrent_detail::node					node;

	WBL_STATIC_ASSERT(concurrent_detail::is_concurrent_node_factory_allow_type<T>::value);

	concurrent_node_factory(const concurrent_node_factory&);
	concurrent_node_factory& operator=(const concurrent_node_factory&);
protected:
	inline concurrent_node_factory()	{}
	inline ~concurrent_node_factory()	{}
	inline void destroy_node(node * n)	{ free_stack_t::_w._free.push(n); }
	node * create_node(const T& v)
	{
		concurrent_detail::free_stack_wrapper<block_size>& w = free_stack_t::_w;
		node_impl * p = (node_impl *) w._free.pop();
		if (p == NULL)
			p = (node_impl *) w._alloc.allocate(block_size);
		p->item = v;
		p->next = NULL;
		return (node *) p;
	}
};

}

#endif

