
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



#include "concurrent_node_factory.h"
#include <string.h>
#include <assert.h>

namespace wbl
{
namespace concurrent_detail
{
	void free_all_node(node_stack& s, std::allocator<char>& a, int block_size)
	{
		while (char * p = (char *) s.pop())
			a.deallocate(p, block_size);
	}

	namespace
	{
		union helper
		{
			size_t	ver;
			struct
			{
#if __WORDSIZE == 64 || defined(__x86_64__)
				uint32_t	v;				///< 版本号
				uint32_t	count;			///< 元素个数
#else
				size_t	v : 8;
				size_t	count : 24;
#endif
			};
		};

		inline bool cas_top(volatile ptr_wrap& top, const ptr_wrap& old, node * n, size_t c)
		{
			helper h;
			h.ver = old.ver;
			h.count += c;
			ptr_wrap px = {n, h.ver};
			return top.cas(old, px);
		}
	}

	bool node_stack::peek(void * p, size_t len)
	{
		for (; ;) {
			ptr_wrap t = { _top.ptr, _top.ver} ;
			if (t.ptr == NULL)
				return false;
			::memcpy(p, t.ptr->get_item_ptr(), len);
			if (_top.cas(t, t))
				return true;
		}
	}

	node * node_stack::pop()
	{
		for (; ;) {
			//ptr_wrap t = _top;
			ptr_wrap t = { _top.ptr, _top.ver} ;
			if (t.ptr == NULL)
				return NULL;
			node * n = t.ptr->next;
			assert(n != t.ptr);
			if (cas_top(_top, t, n, -1)) {
				return t.ptr;
			}
		}
	}

	void node_stack::push(node * n)
	{
		for (; ;) {
			//ptr_wrap t = _top;
			ptr_wrap t = { _top.ptr, _top.ver} ;
			n->next = t.ptr;
			if (cas_top(_top, t, n, 1)) {
				return;
			}
		}
	}

	node_stack::size_type node_stack::size() const
	{
		helper h;
		h.ver = _top.ver;
		return h.count;
	}
}
}

