
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



#include "concurrent_queue.h"
#include <string.h>
#include <assert.h>

namespace wbl
{
namespace concurrent_detail
{
	long _invalid_ptr = 1;

	void node_list::init(node * init_node)
	{
		head.ptr = tail.ptr = init_node;
		head.ver = tail.ver = 0;
		head.ptr->next = (node *) invalid_node_ptr();
	}

	node * node_list::pop(void * p, size_t len)
	{
		for (; ;) {
			ptr_wrap h = { head.ptr, head.ver };
			ptr_wrap t = { tail.ptr, tail.ver };
			node * first = h.ptr->next;
			if (h == head) {
				if (h.ptr == t.ptr) {
					if (!is_valid_node_ptr(first))
						return NULL;
					tail.cas(t, first);
				} else if (is_valid_node_ptr(first)) {	// 当h和t赋值后，node重用一圈导致h.ptr == head.ptr时，有可能出现first为NULL切h.ptr == t.ptr的情况
					::memcpy(p, first->get_item_ptr(), len);
					//v = first->get_item<T>();	// 由于节点需要重用，因此赋值操作只能放在head.cas之前
					if (head.cas(h, first)) {
						return h.ptr;
					}
				}
			}
		}
	}

	node * node_list::pop()
	{
		for (; ;) {
			//ptr_wrap h = head;
			//ptr_wrap t = tail;
			ptr_wrap h = { head.ptr, head.ver };
			ptr_wrap t = { tail.ptr, tail.ver };
			node * first = h.ptr->next;
			if (h == head) {
				if (h.ptr == t.ptr) {
					if (!is_valid_node_ptr(first))
						return NULL;
					tail.cas(t, first);
				} else if (head.cas(h, first)) {
					return h.ptr;
				}
			}
		}
	}

	bool node_list::peek(void * p, size_t len)
	{
		for (; ;) {
			//ptr_wrap h = head;
			//ptr_wrap t = tail;
			ptr_wrap h = { head.ptr, head.ver };
			ptr_wrap t = { tail.ptr, tail.ver };
			node * first = h.ptr->next;
			if (h == head) {
				if (h.ptr == t.ptr) {
					if (!is_valid_node_ptr(first))
						return false;
					tail.cas(t, first);
				} else if (is_valid_node_ptr(first)) {
					//v = first->get_item<T>();
					::memcpy(p, first->get_item_ptr(), len);
					if (head.cas(h, h))
						return true;
				}
			}
		}
	}

	size_t node_list::size() 
	{
		//node_list x = *this;
		//return x.tail.ver - x.head.ver;
		for (; ;) {
			//ptr_wrap t = tail;
			ptr_wrap t = { tail.ptr, tail.ver };
			node * s = t.ptr->next;
			if (t == tail) {
				if (!is_valid_node_ptr(s)) {
					node_list x = *this;
					return x.tail.ver - x.head.ver;
				} else {
					tail.cas(t, s);
				}
			}
		}
	}

	void node_list::push(node * n)
	{
		n->next = (node *) invalid_node_ptr();
		for (; ;) {
			//ptr_wrap t = tail;
			ptr_wrap t = { tail.ptr, tail.ver };
			node * s = t.ptr->next;
			if (t == tail) {
				if (!is_valid_node_ptr(s)) {
					if (atomicity::compare_and_swap(&t.ptr->next, s, n)) {
						assert(n != t.ptr);
						tail.cas(t, n);
						return;
					}
				} else {
					tail.cas(t, s);
				}
			}
		}
	}

	void node_list::push_all(node * first, node * last, size_t count)
	{
		last->next = (node *) invalid_node_ptr();
		for (; ;) {
			//ptr_wrap t = tail;
			ptr_wrap t = { tail.ptr, tail.ver };
			node * s = t.ptr->next;
			if (t == tail) {
				if (!is_valid_node_ptr(s)) {
					if (atomicity::compare_and_swap(&t.ptr->next, s, first)) {
						ptr_wrap tn = { last, t.ver + count };
						tail.cas(t, tn);
						return;
					}
				} else {
					tail.cas(t, s);
				}
			}
		}
	}
}
}

