
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


#ifndef __TCE_SHM_HASHMAP__
#define __TCE_SHM_HASHMAP__

#include "tce_def.h"
#include "tce_shm_hash_fun.h"
#include "tce_shm.h"
#include <utility>
#include <math.h>
#include <string>
#include <iostream>
using namespace std;

namespace tce{
namespace shm{

enum KEY_TYPE{
	KT_NUMBER = 0,
	KT_STRING_16 = 16,
	KT_STRING_32 = 32,
	KT_STRING_64 = 64,
	KT_STRING_128 = 128,
	KT_STRING_256 = 256,
	KT_STRING_512 = 512,
	KT_STRING_1024 = 1024,
};


template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
class CHashMap;

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
struct _Hashmap_iterator;

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
struct _Hashmap_const_iterator;

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey> 
struct _Hashmap_key;

#pragma pack(1)
template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
struct _Hashmap_node
{
	typedef _Hashmap_key<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey> key_type;
	friend class CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>;
	friend class _Hashmap_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>;
	friend class _Hashmap_const_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>;
private:
	unsigned char ucFlag;// 1:使用中；0:未使用
public:
	key_type first;
	_Tp second;
}; 
#pragma pack()


template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
struct _Hashmap_key{
	friend class CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>;

	_Key m_key;
	_Hashmap_key()
		:m_key(0)
	{}	
	_Hashmap_key(const _Key& __key)
		:m_key(__key)
	{}
	operator _Key()	{	return m_key;	}
	operator const _Key() const	{	return m_key;	}
	bool IsString() const	{	return false;	}
	std::string asString() const {	return "";	}
private:
};

template<class _Tp, class _Head, class _HashFcn, class _EqualKey>  
struct _Hashmap_key<std::string, _Tp, Int2Type<KT_STRING_16>, _Head, _HashFcn, _EqualKey>{
	friend class CHashMap<std::string, _Tp, Int2Type<KT_STRING_16>, _Head, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_16> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type];
	_Hashmap_key()
		:m_len(0)
	{
		memset(m_key, 0, sizeof(m_key));
	}	
	_Hashmap_key(const key_type& __key)
	{
		if ( __key.size() >= STRING_TYPE::type )
		{
			memcpy(m_key, __key.data(), sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			memcpy(m_key, __key.data(), __key.size());
			m_len = __key.size();
		}
	}
	_Hashmap_key(const char* __key)
	{
		if ( strlen(__key) >= STRING_TYPE::type )
		{
			memcpy(m_key, __key, sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			m_len = (unsigned char)strlen(__key);
			memcpy(m_key, __key, m_len);
		}
	}
	operator key_type()	{	return std::string(m_key, m_len);	}
	operator const key_type() const	{	return std::string(m_key, m_len);	}
	bool IsString() const	{	return true;	}
	std::string asString() const {	return std::string(m_key, m_len);	}
private:
};

template<class _Tp, class _Head, class _HashFcn, class _EqualKey>  
struct _Hashmap_key<std::string, _Tp, Int2Type<KT_STRING_32>, _Head, _HashFcn, _EqualKey>{
	friend class CHashMap<std::string, _Tp, Int2Type<KT_STRING_32>, _Head, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_32> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type];
	_Hashmap_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_Hashmap_key(const key_type& __key)
	{
		if ( __key.size() >= STRING_TYPE::type )
		{
			memcpy(m_key, __key.data(), sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			memcpy(m_key, __key.data(), __key.size());
			m_len = __key.size();
		}
	}
	_Hashmap_key(const char* __key)
	{
		if ( strlen(__key) >= STRING_TYPE::type )
		{
			memcpy(m_key, __key, sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			m_len = (unsigned char)strlen(__key);
			memcpy(m_key, __key, m_len);
		}
	}
	operator key_type()	{	return std::string(m_key, m_len);	}
	operator const key_type() const	{	return std::string(m_key, m_len);	}
	bool IsString() const	{	return true;	}
	std::string asString() const {	return std::string(m_key, m_len);	}
private:
};


template<class _Tp, class _Head, class _HashFcn, class _EqualKey>  
struct _Hashmap_key<std::string, _Tp, Int2Type<KT_STRING_64>, _Head, _HashFcn, _EqualKey>{
	friend class CHashMap<std::string, _Tp, Int2Type<KT_STRING_64>, _Head, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_64> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type];
	_Hashmap_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_Hashmap_key(const key_type& __key)
	{
		if ( __key.size() >= STRING_TYPE::type )
		{
			memcpy(m_key, __key.data(), sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			memcpy(m_key, __key.data(), __key.size());
			m_len = __key.size();
		}
	}
	_Hashmap_key(const char* __key)
	{
		if ( strlen(__key) >= STRING_TYPE::type )
		{
			memcpy(m_key, __key, sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			m_len = (unsigned char)strlen(__key);
			memcpy(m_key, __key, m_len);
		}
	}
	operator key_type()	{	return std::string(m_key, m_len);	}
	operator const key_type() const	{	return std::string(m_key, m_len);	}
	bool IsString() const	{	return true;	}
	std::string asString() const {	return std::string(m_key, m_len);	}
private:
};


template<class _Tp, class _Head, class _HashFcn, class _EqualKey>  
struct _Hashmap_key<std::string, _Tp, Int2Type<KT_STRING_128>, _Head, _HashFcn, _EqualKey>{
	friend class CHashMap<std::string, _Tp, Int2Type<KT_STRING_128>, _Head, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_128> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type];
	_Hashmap_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_Hashmap_key(const key_type& __key)
	{
		if ( __key.size() >= STRING_TYPE::type )
		{
			memcpy(m_key, __key.data(), sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			memcpy(m_key, __key.data(), __key.size());
			m_len = __key.size();
		}
	}
	_Hashmap_key(const char* __key)
	{
		if ( strlen(__key) >= STRING_TYPE::type )
		{
			memcpy(m_key, __key, sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			m_len = (unsigned char)strlen(__key);
			memcpy(m_key, __key, m_len);
		}
	}
	operator key_type()	{	return std::string(m_key, m_len);	}
	operator const key_type() const	{	return std::string(m_key, m_len);	}
	bool IsString() const	{	return true;	}
	std::string asString() const {	return std::string(m_key, m_len);	}
private:
};

template<class _Tp, class _Head, class _HashFcn, class _EqualKey>  
struct _Hashmap_key<std::string, _Tp, Int2Type<KT_STRING_256>, _Head, _HashFcn, _EqualKey>{
	friend class CHashMap<std::string, _Tp, Int2Type<KT_STRING_256>, _Head, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_256> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type-1];
	_Hashmap_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_Hashmap_key(const key_type& __key)
	{
		if ( __key.size() >= STRING_TYPE::type )
		{
			memcpy(m_key, __key.data(), sizeof(m_key));
			m_len = 255;
		}
		else
		{
			memcpy(m_key, __key.data(), __key.size());
			m_len = __key.size();
		}
	}
	_Hashmap_key(const char* __key)
	{
		if ( strlen(__key) >= STRING_TYPE::type )
		{
			memcpy(m_key, __key, sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			m_len = (unsigned char)strlen(__key);
			memcpy(m_key, __key, m_len);
		}
	}
	operator key_type()	{	return std::string(m_key, m_len);	}
	operator const key_type() const	{	return std::string(m_key, m_len);	}
	bool IsString() const	{	return true;	}
	std::string asString() const {	return std::string(m_key, m_len);	}
private:
};


template<class _Tp, class _Head, class _HashFcn, class _EqualKey>  
struct _Hashmap_key<std::string, _Tp, Int2Type<KT_STRING_512>, _Head, _HashFcn, _EqualKey>{
	friend class CHashMap<std::string, _Tp, Int2Type<KT_STRING_512>, _Head, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_512> STRING_TYPE;
	uint16_t m_len;
	char m_key[STRING_TYPE::type];
	_Hashmap_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_Hashmap_key(const key_type& __key)
	{
		if ( __key.size() >= STRING_TYPE::type )
		{
			memcpy(m_key, __key.data(), sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			memcpy(m_key, __key.data(), __key.size());
			m_len = __key.size();
		}
	}
	_Hashmap_key(const char* __key)
	{
		if ( strlen(__key) >= STRING_TYPE::type )
		{
			memcpy(m_key, __key, sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			m_len = (unsigned char)strlen(__key);
			memcpy(m_key, __key, m_len);
		}
	}
	operator key_type()	{	return std::string(m_key, m_len);	}
	operator const key_type() const	{	return std::string(m_key, m_len);	}
	bool IsString() const	{	return true;	}
	std::string asString() const {	return std::string(m_key, m_len);	}
private:
};



template<class _Tp, class _Head, class _HashFcn, class _EqualKey>  
struct _Hashmap_key<std::string, _Tp, Int2Type<KT_STRING_1024>, _Head, _HashFcn, _EqualKey>{
	friend class CHashMap<std::string, _Tp, Int2Type<KT_STRING_1024>, _Head, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_1024> STRING_TYPE;
	uint16_t m_len;
	char m_key[STRING_TYPE::type];
	_Hashmap_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_Hashmap_key(const key_type& __key)
	{
		if ( __key.size() >= STRING_TYPE::type )
		{
			memcpy(m_key, __key.data(), sizeof(m_key));
			m_len = sizeof(m_key);
		}
		else
		{
			memcpy(m_key, __key.data(), __key.size());
			m_len = __key.size();
		}
	}
	operator key_type()	{	return std::string(m_key, m_len);	}
	operator const key_type() const	{	return std::string(m_key, m_len);	}
	bool IsString() const	{	return true;	}
	std::string asString() const {	return std::string(m_key, m_len);	}
private:
};



template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
struct _Hashmap_iterator {
	typedef _Hashmap_key<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey> key_type;
	typedef _Hashmap_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>		iterator;
	typedef _Hashmap_const_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>		const_iterator;
	typedef _Hashmap_node<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey> node_type;
	typedef size_t size_type;
	typedef node_type& reference;
	typedef node_type* pointer;

	node_type* m_cur;
	node_type* m_pEndNode;

	_Hashmap_iterator(node_type* __n, node_type* __end_node) 
		: m_cur(__n),m_pEndNode(__end_node){}
	_Hashmap_iterator() {}
	_Hashmap_iterator(const iterator& __it) 
		: m_cur(__it.m_cur), m_pEndNode(__it.m_pEndNode) {}
	reference operator*() const { return *m_cur; }
	pointer operator->() const { return &(operator*()); }
	inline iterator& operator++();
	inline iterator operator++(int);
	bool operator==(const iterator& __it) const
	{ return m_cur == __it.m_cur; }
	bool operator!=(const iterator& __it) const
	{ return m_cur != __it.m_cur; }
};

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
struct _Hashmap_const_iterator {
	typedef _Hashmap_key<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey> key_type;
	typedef _Hashmap_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>		iterator;
	typedef _Hashmap_const_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>		const_iterator;
	typedef _Hashmap_node<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey> node_type;
	typedef size_t size_type;
	typedef node_type& reference;
	typedef node_type* pointer;

	node_type* m_cur;
	node_type* m_pEndNode;

	_Hashmap_const_iterator(const node_type* __n, node_type* __end_node)
		: m_cur(__n),m_pEndNode(__end_node){}
	_Hashmap_const_iterator() {}
	_Hashmap_const_iterator(const iterator& __it) 
		: m_cur(__it.m_cur), m_pEndNode(__it.m_pEndNode) {}
	reference operator*() const { return m_cur; }
	pointer operator->() const { return &(operator*()); }
	const_iterator& operator++();
	const_iterator operator++(int);
	bool operator==(const const_iterator& __it) const 
	{ return m_cur == __it.m_cur; }
	bool operator!=(const const_iterator& __it) const 
	{ return m_cur != __it.m_cur; }
};





template <class _Key, class _Tp, class _KeyType=Int2Type<KT_NUMBER>, class _Head=SEmptyHead, class _HashFcn=hash<_Key>, class _EqualKey=std::equal_to<_Key> >
class CHashMap
{
public:
	typedef _Hashmap_key<_Key,_Tp,_KeyType,_Head,_HashFcn,_EqualKey> key_type;
	typedef std::pair<const key_type,_Tp> value_type;
	typedef _Hashmap_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>	iterator;
	typedef _Hashmap_const_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey> const_iterator;
	typedef size_t		size_type;
	typedef _Hashmap_node<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey> node_type;
	typedef _HashFcn hasher;
	typedef _EqualKey key_equal;
	typedef _Head head_type;
private:

#pragma pack(4)
	struct SIndex{
		SIndex(){memset(this, 0, sizeof(SIndex));}
		uint32_t dwCRC;				//crc校验，防止stepkey被修改
		uint32_t dwVer;				//版本号
		uint64_t dwShmSize;			//共享内存大小
		uint32_t uiHeadSize;		//用户定义的head大小
		uint64_t dwStepCount;		//阶数
		uint64_t dwStepSize;		//阶的大小
		uint64_t dwNodeBufSize;		//node buf size 增加该字段的目的是：为了防止新增字段为清除shm而导致数据错乱
		uint64_t dwUsingSize;		//已使用的数量
		char szReserve[972];		//保留
		uint32_t arStepMod[1024];	//每阶的mod 值
	};
#pragma pack()

public:
	CHashMap()
		:m_bInit(false)
		,m_pHead(NULL)
		,m_pstIndex(NULL)
		,m_pNodes(NULL)
		,m_pEndNode(NULL)
	{
		memset(m_szErrMsg, 0, sizeof(m_szErrMsg));
	}
	~CHashMap(){

	}

	inline bool init(const int32_t iShmKey, const size_t nShmMaxSize, const bool bReadOnly=false);
	inline std::pair<iterator,bool> insert(const value_type& __obj);
	inline iterator find(const key_type& key);
	inline const_iterator find(const key_type& key) const;
	iterator end() 	{	return iterator(0,m_pEndNode);	}
	const_iterator end() const	{	return const_iterator(0,m_pEndNode);	}
	inline const_iterator begin()	const;
	inline iterator begin() ;
	size_t erase(const key_type& __key);
	size_t erase(iterator __it);
	void clear();
	inline size_t max_size() const {	return m_pstIndex->dwStepCount*m_pstIndex->dwStepSize;	}
	inline size_t size() const {	return m_pstIndex->dwUsingSize;	}

	inline size_t step_count() const {	return m_pstIndex->dwStepCount;	}
	inline size_t step_using_rate(const size_t dwStepIndex);
	inline size_t step_using_size(const size_t dwStepIndex);

	const char* err_msg() const {	return m_szErrMsg;	}

	head_type& head(){	return *m_pHead;	}
	const head_type& head() const {	return *m_pHead;	}

private:
	uint32_t _getHeadCRC();	
	bool IsPrime(const uint64_t dwValue)
	{
		uint64_t dwTmp = (uint64_t)sqrt(dwValue);
		for ( uint64_t i=2; i<dwTmp; ++i )
		{
			if ( dwValue%i == 0 )
				return false;
		}
		return true;
	}

private:
	bool m_bInit;
	head_type* m_pHead;
	SIndex* m_pstIndex;
	SIndex m_stCopyIndex;
	node_type* m_pNodes;
	node_type* m_pEndNode;
	hasher m_hashFun;
	key_equal m_equalFun;
	tce::CShm m_oShm;
	char m_szErrMsg[1024];
};

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::size_type CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::step_using_rate(const size_type dwStepIndex)
{
	if ( dwStepIndex >= m_pstIndex->dwStepCount )
		return 100;

	size_type dwUsingSize = 0;
	node_type* pBegin = (node_type*)(m_pNodes+dwStepIndex*m_pstIndex->dwStepSize);
	for ( uint64_t k=0; k<m_pstIndex->arStepMod[dwStepIndex]; ++k )
	{
		if ( 1 == (pBegin+k)->ucFlag  )
		{
			++dwUsingSize;
		}
	}

	return dwUsingSize*100/m_pstIndex->arStepMod[dwStepIndex];
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::size_type CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::step_using_size(const size_type dwStepIndex)
{
	if ( dwStepIndex >= m_pstIndex->dwStepCount )
		return 0;

	size_type dwUsingSize = 0;
	node_type* pBegin = (node_type*)(m_pNodes+dwStepIndex*m_pstIndex->dwStepSize);
	for ( uint64_t k=0; k<m_pstIndex->arStepMod[dwStepIndex]; ++k )
	{
		if ( 1 == (pBegin+k)->ucFlag  )
		{
			++dwUsingSize;
		}
	}

	return dwUsingSize;
}




template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
uint32_t CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::_getHeadCRC()
{
	//计算除了CRC字段以为的bufCRC值
	SIndex stCopyIndex;
	memcpy(&stCopyIndex, m_pstIndex, sizeof(stCopyIndex));
	stCopyIndex.dwUsingSize = 0;
	stCopyIndex.dwCRC = 0;
	return tce::CRC32((char*)&stCopyIndex, sizeof(stCopyIndex));
}


template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
bool CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::init(const int32_t iShmKey, const size_type dwShmMaxSize, const bool bReadOnly)
{
	if ( m_bInit )
		return true;

	if ( !m_oShm.Init( iShmKey, dwShmMaxSize ) )
	{
		xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMap Init Failed:%s", m_oShm.GetErrMsg());
		return false;
	}

	
	if ( m_oShm.IsCreate() )
	{
		if ( !m_oShm.Attach(false) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMap Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}
		//init shm buf struct 
		//shm index 
		m_pstIndex = (SIndex*)reinterpret_cast<char*>(m_oShm.GetShmBuf());

		size_type dwCount = (dwShmMaxSize-sizeof(SIndex)-sizeof(head_type))/sizeof(node_type);
		m_pstIndex->dwStepCount = 25;
		m_pstIndex->dwStepSize = (uint32_t)(dwCount/25);
		m_pstIndex->dwShmSize = dwShmMaxSize;
		m_pstIndex->dwVer = 1;
		m_pstIndex->uiHeadSize = sizeof(head_type);
		m_pstIndex->dwNodeBufSize = sizeof(node_type);	

		uint32_t dwTmpMod = m_pstIndex->dwStepSize;

		uint32_t dwTotal = 0;
		//create step mod
		for ( int32_t i=0; i<25; ++i )
		{
			//还需要质数的判断函数
			bool bFind = false;
			uint32_t dwFindMod = 0;
			while( dwTmpMod > 0 && !bFind )
			{
				bFind = IsPrime(dwTmpMod);
				dwFindMod = dwTmpMod;
				--dwTmpMod;
			}
			
			if ( bFind )
			{
				m_pstIndex->arStepMod[i] = dwFindMod;
			}
			else
			{
				if ( i> 0 )
					m_pstIndex->arStepMod[i] = m_pstIndex->arStepMod[i-1];
				else
					m_pstIndex->arStepMod[i] = m_pstIndex->dwStepSize;
			}

	//		cout << "i=" << i << "; mod=" << m_pstIndex->arStepMod[i] << endl;
			dwTotal += m_pstIndex->arStepMod[i];
		}

	//	cout << "max=" << m_pstIndex->dwStepCount*m_pstIndex->dwStepSize << "; real=" << dwTotal << endl;

		//还没有计算CRC值
		m_pstIndex->dwCRC = this->_getHeadCRC();

	//	printf("create shm=0x%x end=0x%x, stepcount=%lu,stepsize=%lu.\n", reinterpret_cast<char*>(m_oShm.GetShmBuf()), (char*)((char*)m_oShm.GetShmBuf()+dwShmMaxSize), m_pstIndex->dwStepCount, m_pstIndex->dwStepSize);

		//根据是否只读方式重新绑定共享内存
		if ( !m_oShm.Detach() )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMap Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}
		if ( !m_oShm.Attach(bReadOnly) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMap Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}
	}
	else
	{
		//根据是否只读方式绑定共享内存
		if ( !m_oShm.Attach(bReadOnly) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMap Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}

		//init shm buf struct 
		//shm index 
		m_pstIndex = (SIndex*)reinterpret_cast<char*>(m_oShm.GetShmBuf());

		//检查内存头是否正确
		if ( m_pstIndex->dwCRC != this->_getHeadCRC() )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMap Init Failed:index data error:crc32 no equal.");
			return false;
		}

		if ( m_pstIndex->dwNodeBufSize != sizeof(node_type) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMap Init Failed:index data error:node buf size no equal.");
			return false;
		}

	}

	//printf("shm=0x%x end=0x%x.\n", reinterpret_cast<char*>(m_oShm.GetShmBuf()), (char*)((char*)m_oShm.GetShmBuf()+dwShmMaxSize));


	//copy一份数据到本地内存为了做校验
	memcpy(&m_stCopyIndex, m_pstIndex, sizeof(m_stCopyIndex));
	m_pHead = (head_type*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(SIndex));
	m_pNodes = (node_type*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(SIndex)+sizeof(head_type));
	m_pEndNode = m_pNodes+m_pstIndex->dwStepSize*m_pstIndex->dwStepCount;
	m_bInit = true;
	return true;
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::iterator CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::find(const key_type& __key)
{
	node_type* _value = NULL;
	for ( uint64_t i=0; i<m_pstIndex->dwStepCount; ++i )
	{
		uint64_t ui64StepPos = m_hashFun(__key) % m_pstIndex->arStepMod[i];	
		node_type* tmp  = (node_type*)(m_pNodes+i*m_pstIndex->dwStepSize+ui64StepPos);
		if ( tmp->ucFlag == 1 && m_equalFun(__key, tmp->first) )
		{
			_value = tmp;
			break;
		}
	}
	return iterator(_value,m_pEndNode);
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::const_iterator CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::find(const key_type& __key) const
{
	node_type* _value = NULL;
	for ( uint64_t i=0; i<m_pstIndex->dwStepCount; ++i )
	{
		uint64_t ui64StepPos = m_hashFun(__key) % m_pstIndex->arStepMod[i];	
		node_type* tmp  = (node_type*)(m_pNodes+i*m_pstIndex->dwStepSize+ui64StepPos);
		if ( tmp->ucFlag == 1 && m_equalFun(__key, tmp->first) )
		{
			_value = tmp;
			break;
		}
	}
	return const_iterator(_value,m_pEndNode);
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::size_type CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::erase(iterator __it)
{
	node_type* __p = __it.m_cur;
	if ( 0 != __p &&  1 == __p->ucFlag  )
	{
		memset(__p, 0, sizeof(node_type));
		
		if ( m_pstIndex->dwUsingSize > 0 )
			m_pstIndex->dwUsingSize--;

		return 1;
	}
	return 0;
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::size_type CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::erase(const key_type& __key)
{
	size_type nCount = 0;
	iterator itFind = find(__key);
	if ( itFind != end() )
	{
		nCount =  erase(itFind);
	}
	return nCount;
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::iterator CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::begin()
{
	for (uint64_t i = 0; i < m_pstIndex->dwStepSize*m_pstIndex->dwStepCount; ++i){
		node_type* first = m_pNodes+i;
		if ( 1 == first->ucFlag  )
		{
			return iterator(first,m_pEndNode);
		}
	}
	return end();
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::const_iterator CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::begin() const
{
	for (uint64_t i = 0; i < m_pstIndex->dwStepSize*m_pstIndex->dwStepCount; ++i){
		node_type* first = m_pNodes+i;
		if ( 1 == first->ucFlag  )
		{
			return const_iterator(first,m_pEndNode);
		}
	}
	return end();
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
void CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::clear()
{
//	printf("clear begin=0x%x,end=0x%x.\n", (char*)m_pNodes, (char*)m_pNodes+(sizeof(node_type)*m_pstIndex->dwStepSize*m_pstIndex->dwStepCount));
	memset((char*)m_pNodes, 0, sizeof(node_type)*m_pstIndex->dwStepSize*m_pstIndex->dwStepCount);
	m_pstIndex->dwUsingSize = 0;
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
_Hashmap_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>&	_Hashmap_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::operator++()
{
	node_type* __old = m_cur;
	m_cur = NULL;
	for (node_type* i = __old+1; i < m_pEndNode; ++i){
		if ( 1 == i->ucFlag  )
		{
			m_cur = i;
			break;
		}
	}
	return *this;
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
	inline _Hashmap_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>
	_Hashmap_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::operator++(int)
{
	iterator __tmp = *this;
	++*this;
	return __tmp;
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
	_Hashmap_const_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>&
	_Hashmap_const_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::operator++()
{
	const node_type* __old = m_cur;
	m_cur = NULL;
	for (node_type* i = __old+1; i < m_pEndNode; ++i){
		if ( 1 == i->ucFlag  )
		{
			m_cur = i;
			break;
		}
	}
	return *this;
}

template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
	inline _Hashmap_const_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>
	_Hashmap_const_iterator<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::operator++(int)
{
	const_iterator __tmp = *this;
	++*this;
	return __tmp;
}


template <class _Key, class _Tp, class _KeyType, class _Head, class _HashFcn, class _EqualKey>
std::pair<typename CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::iterator,bool> CHashMap<_Key, _Tp, _KeyType, _Head, _HashFcn, _EqualKey>::insert(const value_type& __obj)
{
	std::pair<iterator, bool> pairResult;
	pairResult.second = false;
	iterator itFind = find(__obj.first);
	if ( itFind != end() )
	{
		pairResult.second = true;
		itFind->second = __obj.second;
		pairResult.first = itFind;
	}
	else
	{
		//new node
		node_type* pNewNode = NULL;
		for ( uint64_t i=0; i<m_pstIndex->dwStepCount; ++i )
		{
			uint64_t ui64StepPos = m_hashFun(__obj.first) % m_pstIndex->arStepMod[i];	
			pNewNode  = (node_type*)(m_pNodes+i*m_pstIndex->dwStepSize+ui64StepPos);
			if ( pNewNode->ucFlag == 0 )
			{
				pNewNode->first = __obj.first;
				pNewNode->second = __obj.second;
				pNewNode->ucFlag = 1;
				pairResult.second = true;
				pairResult.first = iterator(pNewNode,m_pEndNode);
				m_pstIndex->dwUsingSize++;
				break;
			}
		}

	}

	return pairResult;
}



};

};


#endif
