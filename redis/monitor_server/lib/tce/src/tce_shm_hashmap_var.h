
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


#ifndef __TCE_SHM_HASHMAP_VAR__
#define __TCE_SHM_HASHMAP_VAR__

#include "tce_def.h"
#include "tce_shm_hash_fun.h"
#include "tce_shm_hashmap.h"
#include "tce_shm.h"
#include <utility>
#include <math.h>
#include <string>
#include <iostream>
#include <assert.h>
using namespace std;

namespace tce{
namespace shm{

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
class CHashMapVar;

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
struct _HashmapVar_iterator;

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
struct _HashmapVar_const_iterator;

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey> 
struct _HashmapVar_key;

#pragma pack(1)
template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
struct _HashmapVar_node
{
	typedef _HashmapVar_key<_Key,_KeyType,_HashFcn,_EqualKey> key_type;
	friend class CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>;
	friend class _HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>;
	friend class _HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey>;
	typedef size_t size_type;
private:
	unsigned char ucFlag;// 1:使用中；0:未使用
	size_type nCRC32;	//crc32 只校验data内容
	key_type key;
	size_type nDataSize;//data大小
	size_type nDataPtr;//data在共享内存的块开始值
}; 
#pragma pack()

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
struct _HashmapVar_key{
	friend class CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>;

	_Key m_key;
	_HashmapVar_key()
		:m_key(0)
	{}	
	_HashmapVar_key(const _Key& __key)
		:m_key(__key)
	{}
	operator _Key()	{	return m_key;	}
	operator const _Key() const	{	return m_key;	}
	bool IsString() const	{	return false;	}
	std::string asString() const {	return "";	}
private:
};

template<class _HashFcn, class _EqualKey>  
struct _HashmapVar_key<std::string, Int2Type<KT_STRING_16>, _HashFcn, _EqualKey>{
	friend class CHashMapVar<std::string, Int2Type<KT_STRING_16>, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_16> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type];
	_HashmapVar_key()
		:m_len(0)
	{
		memset(m_key, 0, sizeof(m_key));
	}	
	_HashmapVar_key(const key_type& __key)
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
	_HashmapVar_key(const char* __key)
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

template<class _HashFcn, class _EqualKey>  
struct _HashmapVar_key<std::string, Int2Type<KT_STRING_32>, _HashFcn, _EqualKey>{
	friend class CHashMapVar<std::string, Int2Type<KT_STRING_32>, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_32> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type];
	_HashmapVar_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_HashmapVar_key(const key_type& __key)
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
	_HashmapVar_key(const char* __key)
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


template<class _HashFcn, class _EqualKey>  
struct _HashmapVar_key<std::string, Int2Type<KT_STRING_64>, _HashFcn, _EqualKey>{
	friend class CHashMapVar<std::string, Int2Type<KT_STRING_64>, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_64> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type];
	_HashmapVar_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_HashmapVar_key(const key_type& __key)
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
	_HashmapVar_key(const char* __key)
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


template<class _HashFcn, class _EqualKey>  
struct _HashmapVar_key<std::string, Int2Type<KT_STRING_128>, _HashFcn, _EqualKey>{
	friend class CHashMapVar<std::string, Int2Type<KT_STRING_128>, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_128> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type];
	_HashmapVar_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_HashmapVar_key(const key_type& __key)
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
	_HashmapVar_key(const char* __key)
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

template<class _HashFcn, class _EqualKey>  
struct _HashmapVar_key<std::string, Int2Type<KT_STRING_256>, _HashFcn, _EqualKey>{
	friend class CHashMapVar<std::string, Int2Type<KT_STRING_256>, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_256> STRING_TYPE;
	unsigned char m_len;
	char m_key[STRING_TYPE::type-1];
	_HashmapVar_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_HashmapVar_key(const key_type& __key)
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
	_HashmapVar_key(const char* __key)
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


template<class _HashFcn, class _EqualKey>  
struct _HashmapVar_key<std::string, Int2Type<KT_STRING_512>, _HashFcn, _EqualKey>{
	friend class CHashMapVar<std::string, Int2Type<KT_STRING_512>, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_512> STRING_TYPE;
	unsigned short m_len;
	char m_key[STRING_TYPE::type];
	_HashmapVar_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_HashmapVar_key(const key_type& __key)
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
	_HashmapVar_key(const char* __key)
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



template<class _HashFcn, class _EqualKey>  
struct _HashmapVar_key<std::string, Int2Type<KT_STRING_1024>, _HashFcn, _EqualKey>{
	friend class CHashMapVar<std::string, Int2Type<KT_STRING_1024>, _HashFcn, _EqualKey>;
	typedef std::string key_type;
	typedef Int2Type<KT_STRING_1024> STRING_TYPE;
	unsigned short m_len;
	char m_key[STRING_TYPE::type];
	_HashmapVar_key()
		:m_len(0)
	{		memset(m_key, 0, sizeof(m_key));	}	
	_HashmapVar_key(const key_type& __key)
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



template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
struct _HashmapVar_iterator {
	typedef _HashmapVar_key<_Key,_KeyType,_HashFcn,_EqualKey> key_type;
	typedef _HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>		iterator;
	typedef _HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey>		const_iterator;
	typedef CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey> _HashmapVar;
	typedef _HashmapVar_node<_Key,_KeyType,_HashFcn, _EqualKey> node_type;
	typedef size_t size_type;

	node_type* m_cur;
	node_type* m_pEndNode;
	_HashmapVar* m_hm;


	inline key_type first()
	{
		return m_cur->key;
	}
	inline std::string second(){
		std::string sData;
		sData.resize(m_cur->nDataSize);
		if ( !m_hm->_get_real_data(m_cur->nDataPtr, m_cur->nDataSize, m_cur->nCRC32, (char*)sData.data()) )
		{
			return "";
		}
		return sData;
	}
	inline bool second_get(char* pBuf, size_type& nLen){
		std::string sData;
		if ( nLen < m_cur->nDataSize )
			return false;

		nLen = m_cur->nDataSize;
		if ( !m_hm->_get_real_data(m_cur->nDataPt, m_cur->nDataSize, m_cur->nCRC32, (char*)pBuf) )
		{
			return false;
		}
		return true;
	}

	inline bool second_set(const char* pData, const size_type nSize){


		//new block and copy data
		size_type nDataPtr = m_hm->_new_block(pData, nSize);
		if ( nDataPtr != 0 )
		{
			size_type nTmpPtr = m_cur->nDataPtr;
			m_cur->nDataPtr = nDataPtr;
			m_cur->nDataSize = nSize;
			if ( m_hm->m_pHead->bCRC32Check )
				m_cur->nCRC32 = tce::CRC32(pData, nSize);

			m_hm->_delete_block(nTmpPtr);
		}
		else
		{
			xsnprintf(m_hm->m_szErrMsg,sizeof(m_hm->m_szErrMsg),"HashmapVar second_set error: no enough memory(datasize=%lu).", nSize);
			return false;
		}


		return true;
	}

	_HashmapVar_iterator(node_type* __n, node_type* __end_node, _HashmapVar* __hm) 
		: m_cur(__n),m_pEndNode(__end_node),m_hm(__hm){}
	_HashmapVar_iterator() {}
	_HashmapVar_iterator(const iterator& __it) 
		: m_cur(__it.m_cur), m_pEndNode(__it.m_pEndNode),m_hm(__it.m_hm) {}
	inline iterator& operator++();
	inline iterator operator++(int);
	bool operator==(const iterator& __it) const
	{ return m_cur == __it.m_cur; }
	bool operator!=(const iterator& __it) const
	{ return m_cur != __it.m_cur; }
};

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
struct _HashmapVar_const_iterator {
	typedef _HashmapVar_key<_Key,_KeyType,_HashFcn,_EqualKey> key_type;
	typedef _HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>		iterator;
	typedef _HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey>		const_iterator;
	typedef CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey> _HashmapVar;
	typedef _HashmapVar_node<_Key,_KeyType, _HashFcn, _EqualKey> node_type;
	typedef size_t size_type;

	node_type* m_cur;
	node_type* m_pEndNode;
	_HashmapVar* m_hm;

	inline key_type first()
	{
		return m_cur->key;
	}

	inline std::string second(){
		std::string sData;
		sData.resize(m_cur->nDataSize);
		if ( !m_hm->_get_real_data(m_cur->nDataPt, m_cur->nDataSize, m_cur->nCRC32, (char*)sData.data()) )
		{
			return "";
		}
		return sData;
	}
	inline bool second_get(char* pBuf, size_type& nLen){
		std::string sData;
		if ( nLen < m_cur->nDataSize )
			return false;

		nLen = m_cur->nDataSize;
		if ( !m_hm->_get_real_data(m_cur->nDataPt, m_cur->nDataSize, m_cur->nCRC32, (char*)pBuf) )
		{
			return false;
		}
		return true;
	}

	inline bool second_set(const char* pData, const size_type nSize){
		return true;
	}

	_HashmapVar_const_iterator(const node_type* __n, node_type* __end_node, const _HashmapVar* __hm)
		: m_cur(__n),m_pEndNode(__end_node),m_hm(__hm){}
	_HashmapVar_const_iterator() {}
	_HashmapVar_const_iterator(const iterator& __it) 
		: m_cur(__it.m_cur), m_pEndNode(__it.m_pEndNode),m_hm(__it.m_hm) {}
	const_iterator& operator++();
	const_iterator operator++(int);
	bool operator==(const const_iterator& __it) const 
	{ return m_cur == __it.m_cur; }
	bool operator!=(const const_iterator& __it) const 
	{ return m_cur != __it.m_cur; }
};





template <class _Key, class _KeyType=Int2Type<KT_NUMBER>, class _HashFcn=hash<_Key>, class _EqualKey=std::equal_to<_Key> >
class CHashMapVar
{
	friend class _HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>;
	friend class _HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey>;
public:
	typedef _HashmapVar_key<_Key,_KeyType,_HashFcn,_EqualKey> key_type;
//	typedef std::pair<const key_type> value_type;
	typedef _HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>	iterator;
	typedef _HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey> const_iterator;
	typedef unsigned long		size_type;
	typedef _HashmapVar_node<_Key,_KeyType,_HashFcn,_EqualKey> node_type;
	typedef _HashFcn hasher;
	typedef _EqualKey key_equal;
private:

#pragma pack(1)
	struct SHead{
		SHead(){memset(this, 0, sizeof(SHead));}
		size_type nCRC;			//crc校验，防止stepkey被修改
		size_type nVer;			//版本号
		size_type nShmSize;		//共享内存大小
		size_type nHashSize;		//hash node总个数
		size_type nStepCount;		//阶数
		size_type nStepSize;		//阶的大小
		size_type nNodeBufSize;	//node buf size 增加该字段的目的是：为了防止新增字段为清除shm而导致数据错乱
		size_type nUsingSize;		//已使用的数量
		bool bCRC32Check;			//是否使用crc32进行校验
		size_type nMaxCacheDataSize;//总共的block内存
		size_type nBlockDataSize;	//每块的内存大小
		size_type nTotalBlockCount;	//总共的block梳理
		size_type nEmptyBlockCount;	//未使用的block数量
		size_type nEmptyHead;		//未使用的block块头指针
		char szReserve[975];		//保留
		size_type arStepMod[1024];	//每阶的mod 值
	};

	struct SBlockDataHead{
		size_type nNextPtr;
	};
#pragma pack()
	typedef SBlockDataHead bdh_type;

public:
	CHashMapVar()
		:m_bInit(false)
		,m_pHead(NULL)
		,m_pNodes(NULL)
		,m_pEndNode(NULL)
		,m_pDatas(NULL)
	{
		memset(m_szErrMsg, 0, sizeof(m_szErrMsg));
	}
	~CHashMapVar(){

	}

	bool init(const size_type nShmKey, const size_type nHashSize, const size_type nMaxCacheDataSize, const size_type nBlockDataSize, const bool bCRC32Check=false, const bool bReadOnly=false);

	//inline std::pair<iterator,bool> insert(const key_type& key, const std::string& data);
	inline std::pair<iterator,bool> insert(const key_type& key, const char* data, const size_type size);
	inline iterator find(const key_type& key);
	inline const_iterator find(const key_type& key) const;
	iterator end() 	{	return iterator(0,m_pEndNode, this);	}
	const_iterator end() const	{	return const_iterator(0,m_pEndNode);	}
	inline const_iterator begin()	const;
	inline iterator begin() ;
	size_type erase(const key_type& __key);
	size_type erase(iterator __it);
	void clear();
	inline size_type max_size() const {	return m_pHead->nStepCount*m_pHead->nStepSize;	}
	inline size_type size() const {	return m_pHead->nUsingSize;	}

	inline size_type step_count() const {	return m_pHead->nStepCount;	}
	inline size_type step_using_rate(const size_type dwStepIndex);
	inline size_type step_using_size(const size_type dwStepIndex);

	const char* err_msg() const {	return m_szErrMsg;	}

private:
	inline size_type _getHeadCRC();	
	inline node_type* _new_node(const key_type& key);
	inline bool _delete_node(node_type* pNode);

	inline size_type _new_block(const char* pData, const size_type nSize);
	inline bool _delete_block(const size_type nDeletePtr);

	inline size_type _get_next_ptr(const size_type nPtr){
		assert(0!=nPtr);
		return _get_data_head(nPtr)->nNextPtr;
	}
	inline bdh_type* _get_data_head(const size_type nPtr){
		assert(0!=nPtr);
		return  (bdh_type*)(m_pDatas+((nPtr-1)*m_pHead->nBlockDataSize));
	}
	inline char* _get_data(const size_type nPtr){
		assert(0!=nPtr);
		return  m_pDatas+((nPtr-1)*m_pHead->nBlockDataSize) + sizeof(bdh_type);
	}
	inline bool _get_real_data(const size_type nPtr, const size_type nSize, const size_type nCRC32, char* pBuf){
		size_type nNeedSize = nSize;
		size_type nCur = nPtr;
		while( 0 != nCur && 0 != nNeedSize )
		{
			char* pData = _get_data(nCur);
			if ( nNeedSize >= m_pHead->nBlockDataSize-sizeof(bdh_type) )
			{
				memcpy(pBuf+nSize-nNeedSize, pData, m_pHead->nBlockDataSize-sizeof(bdh_type));
				nNeedSize -= m_pHead->nBlockDataSize-sizeof(bdh_type);
				nCur = _get_data_head(nCur)->nNextPtr;
			}
			else
			{
				memcpy(pBuf+nSize-nNeedSize, pData, nNeedSize);
				nNeedSize = 0;
			}
		}

		if ( m_pHead->bCRC32Check )
		{
			if ( nCRC32 != tce::CRC32(pBuf, nSize) )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashmapVar get error: crc32 check error.");
				assert(nCRC32 == tce::CRC32(pBuf, nSize));
				return false;
			}
		}

		return true;
	}
	
	bool IsPrime(const uint32_t dwValue)
	{
		uint32_t dwTmp = (uint32_t)sqrt(dwValue);
		for ( uint32_t i=2; i<dwTmp; ++i )
		{
			if ( dwValue%i == 0 )
				return false;
		}
		return true;
	}
private:
	bool m_bInit;
	SHead* m_pHead;
	SHead m_stCopyHead;
	node_type* m_pNodes;
	node_type* m_pEndNode;
	char* m_pDatas;
	hasher m_hashFun;
	key_equal m_equalFun;
	tce::CShm m_oShm;
	char m_szErrMsg[1024];
};


template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
bool CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::_delete_block(const size_type nDeletePtr)
{
	if ( nDeletePtr == 0 )
		return false;

	size_type nCur = nDeletePtr;
	size_type nCount = 1;
	while(_get_next_ptr(nCur))
	{
		++nCount;
//			cout << "delete nCur=" << nCur << endl;
		nCur = _get_next_ptr(nCur);
	}
	
	_get_data_head(nCur)->nNextPtr = m_pHead->nEmptyHead;
	m_pHead->nEmptyHead = nDeletePtr;
	m_pHead->nEmptyBlockCount += nCount;

	return true;
}

//return:>0 item data index; 0:error.
template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::size_type CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::_new_block(const char* pData, const size_type nSize)
{
	size_type nNeedBlockCount = (nSize+m_pHead->nBlockDataSize-sizeof(bdh_type)-1)/(m_pHead->nBlockDataSize-sizeof(bdh_type));

	if ( m_pHead->nEmptyBlockCount < nNeedBlockCount )
	{//没有足够的空间
		return 0;
	}
	
	size_type nCopySize = 0;
	size_type nBeginItem = m_pHead->nEmptyHead;
	size_type nEndItem = 0;
	size_type nCurItem = nBeginItem;
//		cout << "nNeedBlockCount=" << nNeedBlockCount << endl;
	for ( size_type i=0; i<nNeedBlockCount; ++i )
	{
		assert(0 != nCurItem);
		if ( 0 != nCurItem )
		{
			char* pBuf = _get_data(nCurItem);
//				cout << "i=" << i << "; nCurItem=" << nCurItem << "; pBuf="<< (int)pBuf <<  endl;
			if ( nSize - nCopySize > m_pHead->nBlockDataSize-sizeof(bdh_type) )
			{	
				memcpy(pBuf, pData+nCopySize, m_pHead->nBlockDataSize-sizeof(bdh_type));
				nCurItem = _get_data_head(nCurItem)->nNextPtr;
				nCopySize += m_pHead->nBlockDataSize-sizeof(bdh_type);
			}
			else
			{
				assert(i == nNeedBlockCount-1);
				if ( i != nNeedBlockCount-1 )//分配错误。。
					return 0;

				memcpy(pBuf, pData+nCopySize, nSize - nCopySize);
				nEndItem = _get_data_head(nCurItem)->nNextPtr;

				m_pHead->nEmptyHead = nEndItem;
				_get_data_head(nCurItem)->nNextPtr = 0;

				nCopySize = nSize;
				
				assert(m_pHead->nEmptyBlockCount>=nNeedBlockCount);
				m_pHead->nEmptyBlockCount -= nNeedBlockCount;
			}

		}
		else
		{
			//没有足够的空间
			return 0;
		}
	}

	return nBeginItem;
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::size_type CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::step_using_rate(const size_type dwStepIndex)
{
	if ( dwStepIndex >= m_pHead->nStepCount )
		return 100;

	size_type nUsingSize = 0;
	node_type* pBegin = (node_type*)(m_pNodes+dwStepIndex*m_pHead->nStepSize);
	for ( size_type k=0; k<m_pHead->arStepMod[dwStepIndex]; ++k )
	{
		if ( 1 == (pBegin+k)->ucFlag  )
		{
			++nUsingSize;
		}
	}

	return nUsingSize*100/m_pHead->arStepMod[dwStepIndex];
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::size_type CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::step_using_size(const size_type dwStepIndex)
{
	if ( dwStepIndex >= m_pHead->nStepCount )
		return 0;

	size_type nUsingSize = 0;
	node_type* pBegin = (node_type*)(m_pNodes+dwStepIndex*m_pHead->nStepSize);
	for ( size_type k=0; k<m_pHead->arStepMod[dwStepIndex]; ++k )
	{
		if ( 1 == (pBegin+k)->ucFlag  )
		{
			++nUsingSize;
		}
	}

	return nUsingSize;
}




template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::size_type CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::_getHeadCRC()
{
	//计算除了CRC字段以为的bufCRC值
	SHead stCopyHead;
	memcpy(&stCopyHead, m_pHead, sizeof(stCopyHead));
	stCopyHead.nUsingSize = 0;
	stCopyHead.nCRC = 0;
	stCopyHead.nEmptyHead = 0;
	stCopyHead.nEmptyBlockCount = 0;
	return tce::CRC32((char*)&stCopyHead, sizeof(stCopyHead));
}


template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
bool CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::init(const size_type nShmKey, const size_type nHashSize, const size_type nMaxCacheDataSize, const size_type nBlockDataSize, const bool bCRC32Check/*=false*/, const bool bReadOnly/*=false*/)
{
	if ( m_bInit )
		return true;

	size_type nShmMaxSize = sizeof(SHead) + nHashSize*sizeof(node_type) + nMaxCacheDataSize;
	if ( !m_oShm.Init( (int)nShmKey, nShmMaxSize ) )
	{
		xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMapVar Init Failed:%s", m_oShm.GetErrMsg());
		return false;
	}
	
	if ( m_oShm.IsCreate() )
	{
		if ( !m_oShm.Attach(false) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMapVar Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}
		//init shm buf struct 
		//shm head 
		m_pHead = (SHead*)reinterpret_cast<char*>(m_oShm.GetShmBuf());

		m_pHead->nStepCount = 25;
		m_pHead->nStepSize = nHashSize/25;
		m_pHead->nShmSize = nShmMaxSize;
		m_pHead->nVer = 1;
		m_pHead->nNodeBufSize = sizeof(node_type);	
		m_pHead->bCRC32Check = bCRC32Check;
		m_pHead->nMaxCacheDataSize = nMaxCacheDataSize;
		m_pHead->nBlockDataSize = nBlockDataSize;
		m_pHead->nTotalBlockCount = nMaxCacheDataSize/nBlockDataSize;
		m_pHead->nEmptyBlockCount = m_pHead->nTotalBlockCount;
		m_pHead->nHashSize = nHashSize;

		size_type dwTmpMod = m_pHead->nStepSize;

		uint32_t dwTotal = 0;
		//create step mod
		for ( size_type i=0; i<25; ++i )
		{
			//还需要质数的判断函数
			bool bFind = false;
			size_type dwFindMod = 0;
			while( dwTmpMod > 0 && !bFind )
			{
				bFind = IsPrime(dwTmpMod);
				dwFindMod = dwTmpMod;
				--dwTmpMod;
			}
			
			if ( bFind )
			{
				m_pHead->arStepMod[i] = dwFindMod;
			}
			else
			{
				if ( i> 0 )
					m_pHead->arStepMod[i] = m_pHead->arStepMod[i-1];
				else
					m_pHead->arStepMod[i] = m_pHead->nStepSize;
			}
			dwTotal += m_pHead->arStepMod[i];
		}

		//把empty block 连起来
		m_pDatas = (reinterpret_cast<char*>(m_oShm.GetShmBuf()) + sizeof(SHead) + nHashSize*sizeof(node_type));
		for ( size_type i=1; i<=m_pHead->nTotalBlockCount; ++i )
		{
			if ( 1 == i )
			{
				m_pHead->nEmptyHead = 1;
				_get_data_head(i)->nNextPtr = 0;
			}
			else
			{
				_get_data_head(i-1)->nNextPtr = i;
				_get_data_head(i)->nNextPtr = 0;
			}
		}


	//	cout << "max=" << m_pHead->nStepCount*m_pHead->nStepSize << "; real=" << dwTotal << endl;

		//还没有计算CRC值
		m_pHead->nCRC = this->_getHeadCRC();

	//	printf("create shm=0x%x end=0x%x, stepcount=%lu,stepsize=%lu.\n", reinterpret_cast<char*>(m_oShm.GetShmBuf()), (char*)((char*)m_oShm.GetShmBuf()+dwShmMaxSize), m_pHead->nStepCount, m_pHead->nStepSize);

		//根据是否只读方式重新绑定共享内存
		if ( !m_oShm.Detach() )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMapVar Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}
		if ( !m_oShm.Attach(bReadOnly) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMapVar Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}
	}
	else
	{
		//根据是否只读方式绑定共享内存
		if ( !m_oShm.Attach(bReadOnly) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMapVar Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}

		//init shm buf struct 
		//shm head 
		m_pHead = (SHead*)reinterpret_cast<char*>(m_oShm.GetShmBuf());

		//检查内存头是否正确
		if ( m_pHead->nCRC != this->_getHeadCRC() )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMapVar Init Failed:head data error:crc32 no equal.");
			return false;
		}

		if ( m_pHead->nNodeBufSize != sizeof(node_type) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashMapVar Init Failed:head data error:node buf size no equal.");
			return false;
		}

	}

	//printf("shm=0x%x end=0x%x.\n", reinterpret_cast<char*>(m_oShm.GetShmBuf()), (char*)((char*)m_oShm.GetShmBuf()+dwShmMaxSize));


	//copy一份数据到本地内存为了做校验
	memcpy(&m_stCopyHead, m_pHead, sizeof(m_stCopyHead));
	m_pNodes = (node_type*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(SHead));
	m_pEndNode = m_pNodes+m_pHead->nStepSize*m_pHead->nStepCount;
	m_pDatas = (reinterpret_cast<char*>(m_oShm.GetShmBuf()) + sizeof(SHead) + nHashSize*sizeof(node_type));
	m_bInit = true;
	return true;
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::iterator CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::find(const key_type& __key)
{
	node_type* _value = NULL;
	for ( size_type i=0; i<m_pHead->nStepCount; ++i )
	{
		size_type nStepPos = static_cast<size_type>(m_hashFun(__key) % m_pHead->arStepMod[i]);	
		node_type* tmp  = (node_type*)(m_pNodes+i*m_pHead->nStepSize+nStepPos);
		if ( tmp->ucFlag == 1 && m_equalFun(__key, tmp->key) )
		{
			_value = tmp;
			break;
		}
	}
	return iterator(_value,m_pEndNode, this);
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::const_iterator CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::find(const key_type& __key) const
{
	node_type* _value = NULL;
	for ( size_type i=0; i<m_pHead->nStepCount; ++i )
	{
		size_type nStepPos = static_cast<size_type>(m_hashFun(__key) % m_pHead->arStepMod[i]);	
		node_type* tmp  = (node_type*)(m_pNodes+i*m_pHead->nStepSize+nStepPos);
		if ( tmp->ucFlag == 1 && m_equalFun(__key, tmp->first) )
		{
			_value = tmp;
			break;
		}
	}
	return const_iterator(_value,m_pEndNode);
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::size_type CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::erase(iterator __it)
{
	return _delete_node(__it.m_cur) ? 1 : 0;
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::size_type CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::erase(const key_type& __key)
{
	size_type nCount = 0;
	iterator itFind = find(__key);
	if ( itFind != end() )
	{
		nCount =  erase(itFind);
	}
	return nCount;
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::iterator CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::begin()
{
	for (size_type i = 0; i < m_pHead->nStepSize*m_pHead->nStepCount; ++i){
		node_type* first = m_pNodes+i;
		if ( 1 == first->ucFlag  )
		{
			return iterator(first,m_pEndNode, this);
		}
	}
	return end();
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::const_iterator CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::begin() const
{
	for (size_type i = 0; i < m_pHead->nStepSize*m_pHead->nStepCount; ++i){
		node_type* first = m_pNodes+i;
		if ( 1 == first->ucFlag  )
		{
			return const_iterator(first,m_pEndNode);
		}
	}
	return end();
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
void CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::clear()
{
//	printf("clear begin=0x%x,end=0x%x.\n", (char*)m_pNodes, (char*)m_pNodes+(sizeof(node_type)*m_pHead->nStepSize*m_pHead->nStepCount));
	//clear node & block
	memset((char*)m_pNodes, 0, m_pHead->nHashSize*sizeof(node_type), + m_pHead->nMaxCacheDataSize);
	//把empty block 连起来
	m_pDatas = (reinterpret_cast<char*>(m_oShm.GetShmBuf()) + sizeof(SHead) + m_pHead->nHashSize*sizeof(node_type));
	for ( size_type i=1; i<=m_pHead->nTotalItemCount; ++i )
	{
		if ( 1 == i )
		{
			m_pHead->nEmptyHead = 1;
			_get_data_head(i)->nNextPtr = 0;
		}
		else
		{
			_get_data_head(i-1)->nNextPtr = i;
			_get_data_head(i)->nNextPtr = 0;
		}
	}

	m_pHead->nUsingSize = 0;
	m_pHead->nEmptyBlockCount = m_pHead->nTotalBlockCount;

}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
_HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>&	_HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>::operator++()
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

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
	inline _HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>
	_HashmapVar_iterator<_Key,_KeyType,_HashFcn,_EqualKey>::operator++(int)
{
	iterator __tmp = *this;
	++*this;
	return __tmp;
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
	_HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey>&
	_HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey>::operator++()
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

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
	inline _HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey>
	_HashmapVar_const_iterator<_Key,_KeyType,_HashFcn,_EqualKey>::operator++(int)
{
	const_iterator __tmp = *this;
	++*this;
	return __tmp;
}


template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::node_type* CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::_new_node(const key_type& key)
{
	node_type* pNewNode = NULL;
	for ( size_type i=0; i<m_pHead->nStepCount; ++i )
	{
		size_type nStepPos = static_cast<size_type>(m_hashFun(key) % m_pHead->arStepMod[i]);	
		pNewNode  = (node_type*)(m_pNodes+i*m_pHead->nStepSize+nStepPos);
		if ( pNewNode->ucFlag == 0 )
		{
			pNewNode->ucFlag = 1;
			m_pHead->nUsingSize++;
			break;
		}
	}

	return pNewNode;
}

template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
bool CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::_delete_node(node_type* pNode)
{
	if ( 0 != pNode &&  1 == pNode->ucFlag  )
	{
		_delete_block(pNode->nDataPtr);
		memset(pNode, 0, sizeof(node_type));
		
		if ( m_pHead->nUsingSize > 0 )
			m_pHead->nUsingSize--;

		return true;
	}
	return false;
}


template <class _Key, class _KeyType, class _HashFcn, class _EqualKey>
std::pair<typename CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::iterator,bool> CHashMapVar<_Key,_KeyType,_HashFcn,_EqualKey>::insert(const key_type& key, const char* data, const size_type size)
{
	std::pair<iterator, bool> pairResult;
	pairResult.second = false;
	iterator itFind = find(key);
	if ( itFind != end() )
	{
		pairResult.second = true;
		itFind.second_set(data, size);
		pairResult.first = itFind;
	}
	else
	{
		//new node
		node_type* pNewNode = _new_node(key);
		if ( NULL != pNewNode  )
		{
			//new block and copy data
			size_type nDataPtr = _new_block(data, size);
			if ( nDataPtr != 0 )
			{
				size_type nTmpPtr = pNewNode->nDataPtr;
				pNewNode->key = key;
				pNewNode->nDataPtr = nDataPtr;
				pNewNode->nDataSize = size;
				if ( m_pHead->bCRC32Check )
					pNewNode->nCRC32 = tce::CRC32(data, size);
				_delete_block(nTmpPtr);

				pairResult.second = true;
				pairResult.first = iterator(pNewNode,m_pEndNode, this);
			}
			else
			{
				_delete_node(pNewNode);
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashmapVar insert error: no enough memory(emptysize=%lu,datasize=%lu).", m_pHead->nEmptyBlockCount*(m_pHead->nBlockDataSize-sizeof(bdh_type)), size);
			}
		}
		else
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"HashmapVar insert error: no enough node.");
		}
	}

	return pairResult;
}



};

};


#endif
