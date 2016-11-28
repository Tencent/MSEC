
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
//     Meta Programing Tool
//
//////////////////////////////////////////////////////////////////////////

#ifndef __WBL_META_H__
#define __WBL_META_H__

#define WBL_JOIN(x,y)		WBL_DO_JOIN(x,y)
#define WBL_DO_JOIN(x,y)	WBL_DO_JOIN2(x,y)
#define WBL_DO_JOIN2(x,y)	x##y

namespace wbl
{

template <bool x> struct STATIC_ASSERTION_FAILURE;
template <> struct STATIC_ASSERTION_FAILURE<true> { enum { value = 1 }; };
template<int x> struct static_assert_test{};

#ifdef _MSC_VER					// for vc
#	define WBL_STATIC_ASSERT( B )											\
		typedef ::wbl::static_assert_test<									\
			sizeof(::wbl::STATIC_ASSERTION_FAILURE< (bool)( B ) >)>			\
		WBL_JOIN(wbl_static_assert_typedef_, __COUNTER__)
#else							// for others
#	define WBL_STATIC_ASSERT( B )											\
		typedef ::wbl::static_assert_test<									\
			sizeof(::wbl::STATIC_ASSERTION_FAILURE< (bool)( B ) >)>			\
		WBL_JOIN(wbl_static_assert_typedef_, __LINE__)
#endif

//////////////////////////////////////////////////////////////////////////
// type selector
namespace meta_detail
{
template < bool _F = true >
struct type_selector_helper
{
	template < typename T, typename U >
	struct helper
	{
		typedef T	Type;
		typedef U	Type2;
	};
};

template < >
struct type_selector_helper<false>
{
	template < typename T, typename U >
	struct helper
	{
		typedef U	Type;
		typedef T	type2;
	};
};
}

/** select a type in compile time
 *  if F is true, type_selector<F, T, U>::Type is the first type T in template argument list
 *  otherwise, type_selector<F, T, U>::Type is the second type U
 */
template < bool F, typename T, typename U >
class type_selector
{
public:
	typedef typename meta_detail::type_selector_helper<F>::template helper<T, U>::Type	Type;
	typedef Type	type;
};

//////////////////////////////////////////////////////////////////////////
// type traits
template < typename T >
struct type2type
{
	typedef T type;
};

template < int n >
struct int2type
{
	enum { value = n};
};

template<int N> 
struct type_of_size
{
	char elements[N];
};

typedef type_of_size<1>		yes_type;
typedef type_of_size<2>		no_type;

#define META_BOOL_TYPE_TRAITS_IMPL(cn, tn, val)			\
	template <>											\
	struct cn < tn >									\
	{													\
		enum { value = (val) };							\
	};

namespace meta_detail
{
	using wbl::yes_type;
	using wbl::no_type;

	template <class T>
	T&(* is_reference_helper1(type2type<T>) )(type2type<T>);

	char is_reference_helper1(...);

	template <class T>
	no_type is_reference_helper2(T&(*)(type2type<T>));

	yes_type is_reference_helper2(...);
}

template <typename T>
struct is_reference_type
{
	enum { value = sizeof(meta_detail::is_reference_helper2(
                meta_detail::is_reference_helper1(type2type<T>()))) == sizeof(yes_type)
	};
};

META_BOOL_TYPE_TRAITS_IMPL(is_reference_type, void, false)
META_BOOL_TYPE_TRAITS_IMPL(is_reference_type, void const, false)
META_BOOL_TYPE_TRAITS_IMPL(is_reference_type, void volatile, false)
META_BOOL_TYPE_TRAITS_IMPL(is_reference_type, void const volatile, false)

namespace meta_detail
{
	yes_type is_const_type_func(void volatile const *);
	no_type is_const_type_func(void volatile *);

	template < bool isRef = true >
	struct is_const_type_impl
	{
		template < typename T >
		struct helper
		{
			enum { value = false };
		};
	};

	template <>
	struct is_const_type_impl<false>
	{
		template < typename T >
		struct helper
		{
			static T&    t;
			enum { value = sizeof(is_const_type_func(&t)) == sizeof(yes_type) };
		};
	};
}

template <typename T>
struct is_const_type
{
    enum { value = meta_detail::is_const_type_impl<is_reference_type<T>::value>::template helper<T>::value };
};

META_BOOL_TYPE_TRAITS_IMPL(is_const_type, void, false)
META_BOOL_TYPE_TRAITS_IMPL(is_const_type, void const, true)
META_BOOL_TYPE_TRAITS_IMPL(is_const_type, void volatile, false)
META_BOOL_TYPE_TRAITS_IMPL(is_const_type, void const volatile, true)

namespace meta_detail
{
	template < typename U >
	yes_type is_pointer_type_func(U const volatile *);
	no_type is_pointer_type_func(...);

	template < bool isRef = true >
	struct is_pointer_type_impl
	{
		template < typename T >
		struct helper
		{
			enum { value = false };
		};
	};

	template <>
	struct is_pointer_type_impl<false>
	{
		template < typename T >
		struct helper
		{
			static T&    t;
			enum { value = sizeof(is_pointer_type_func(t)) == sizeof(yes_type) };
		};
	};
}

template <typename T>
struct is_pointer_type
{
	enum { value = meta_detail::is_pointer_type_impl<is_reference_type<T>::value>::template helper<T>::value };
};

META_BOOL_TYPE_TRAITS_IMPL(is_pointer_type, void, false)
META_BOOL_TYPE_TRAITS_IMPL(is_pointer_type, void const, false)
META_BOOL_TYPE_TRAITS_IMPL(is_pointer_type, void volatile, false)
META_BOOL_TYPE_TRAITS_IMPL(is_pointer_type, void const volatile, false)
 
namespace meta_detail
{
	yes_type is_volatile_type_func(void const volatile *);
	no_type is_volatile_type_func(void const *);

	template < bool isRef = true >
	struct is_volatile_type_impl
	{
		template < typename T >
		struct helper
		{
			enum { value = false };
		};
	};

	template <>
	struct is_volatile_type_impl<false>
	{
		template < typename T >
		struct helper
		{
			static T&    t;
			enum { value = sizeof(is_volatile_type_func(&t)) == sizeof(yes_type) };
		};
	};
}

template <typename T>
struct is_volatile_type
{
	enum { value = meta_detail::is_volatile_type_impl<is_reference_type<T>::value>::template helper<T>::value };
};

META_BOOL_TYPE_TRAITS_IMPL(is_volatile_type, void, false)
META_BOOL_TYPE_TRAITS_IMPL(is_volatile_type, void const, false)
META_BOOL_TYPE_TRAITS_IMPL(is_volatile_type, void volatile, true)
META_BOOL_TYPE_TRAITS_IMPL(is_volatile_type, void const volatile, true)

//////////////////////////////////////////////////////////////////////////
// is_convertible
namespace meta_detail
{
	struct any_conversion
	{
		template <typename T> any_conversion(const volatile T&);
		template <typename T> any_conversion(T&);
	};

	template <typename T> struct conversion_checker
	{
		static no_type _m_check(any_conversion ...);
		static yes_type _m_check(T, int);
	};
}

template <typename From, typename To> 
class is_convertible
{
    static From _m_from;
public:
    enum { value = sizeof( meta_detail::conversion_checker<To>::_m_check(_m_from, 0) )
        == sizeof(yes_type) };
};

//////////////////////////////////////////////////////////////////////////
// is_same_type
template < typename T, typename U >
struct is_same_type
{
	enum { value = is_convertible< type2type<T>, type2type<U> >::value };
};

//////////////////////////////////////////////////////////////////
// disable_if
template <bool B, class T = void>
struct disable_if_c
{
	typedef T type;
};

template <class T>
struct disable_if_c<true, T>
{};

template <class Cond, class T = void> 
struct disable_if : public disable_if_c<Cond::value, T>
{};

struct null_type {};

template < typename T >
struct is_null_type
{
	enum { value = is_same_type<T, null_type>::value };
};

}

#undef META_BOOL_TYPE_TRAITS_IMPL

#endif

