
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
 * reference wrapper
 */

#ifndef __WBL_REF_H__
#define __WBL_REF_H__

#include "meta.h"

namespace wbl
{

template<class T>
class reference_wrapper
{ 
public:
    typedef T type;

    explicit reference_wrapper(T& t): _t(&t) {}

    operator T& () const { return *_t; }
    T& get() const { return *_t; }
    T* get_pointer() const { return _t; }
private:
    T* _t;
};

template<class T>
inline reference_wrapper<T> ref(T & t)
{ 
    return reference_wrapper<T>(t);
}

template<class T>
inline reference_wrapper<T const> cref(T const & t)
{
    return reference_wrapper<T const>(t);
}

namespace ref_detail
{
	using wbl::no_type;
	using wbl::yes_type;
	using wbl::type2type;

	no_type is_reference_wrapper_test(...);

	template < typename T >
	yes_type is_reference_wrapper_test(type2type< reference_wrapper<T> >);
}

template < typename T >
struct is_reference_wrapper
{
	enum { value = (sizeof(ref_detail::is_reference_wrapper_test(type2type<T>())) == sizeof(yes_type)) };
};

} // namespace wbl

#endif

