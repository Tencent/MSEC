
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


#ifndef __TCE_VALUE_H__
#define __TCE_VALUE_H__

#include <string>
#include "tce_utils.h"

namespace tce{

class CValue
{
public:

	CValue(const std::string& sValue)
		:m_sValue(sValue)
	{}
	CValue(const char* pValue)
		:m_sValue(pValue)
	{}
	CValue(const CValue& rhs)
		:m_sValue(rhs.m_sValue)
	{}
	CValue(const int16_t _value)
		:m_sValue(ToStr(_value))
	{}
	CValue(const uint16_t _value)
		:m_sValue(ToStr(_value))
	{}
	CValue(const int32_t _value)
		:m_sValue(ToStr(_value))
	{}
	CValue(const uint32_t _value)
		:m_sValue(ToStr(_value))
	{}	
	CValue(const int8_t _value)
		:m_sValue(ToStr(_value))
	{}
	CValue(const uint8_t _value)
		:m_sValue(ToStr(_value))
	{}	
		
#ifdef __x86_64__
#elif __i386__		
	CValue(const unsigned long _value)
		:m_sValue(ToStr(_value))
	{}
	CValue(const long _value)
		:m_sValue(ToStr(_value))
	{}
#endif
		
	CValue& operator=(const CValue& rhs){
		if ( this != &rhs )
		{
			m_sValue = rhs.m_sValue;
		}
		return *this;
	}

	inline int64_t asInt() const {		return atoll(m_sValue.c_str());	}
	inline const std::string& asString() const {		return m_sValue;	}
	
	inline operator uint64_t() const {	return atoll(m_sValue.c_str());	}
	inline operator int64_t() const {	return atoll(m_sValue.c_str());	}
	inline operator unsigned char() const {	return static_cast<unsigned char>(asInt());	}
	inline operator char() const {		return static_cast<char>(asInt());	}
	inline operator uint16_t() const {	return static_cast<uint16_t>(asInt());	}
	inline operator int16_t() const {	return static_cast<int16_t>(asInt());	}
#ifdef __x86_64__
#elif __i386__	
	inline operator unsigned long() const {	return static_cast<unsigned long>(asInt());	}
	inline operator long() const {	return static_cast<long>(asInt());	}
#endif
	
	inline operator int32_t() const {	return static_cast<int32_t>(asInt());	}
	inline operator uint32_t() const {	return static_cast<uint32_t>(asInt());	}
	inline operator bool() const {	return asInt()==0 ? false : true;	}
	inline operator std::string() const {	return m_sValue;	}

	const char* data() const {	return m_sValue.data();	}
	const size_t size() const {	return m_sValue.size();	}

private:
	std::string m_sValue;
};

}


#endif

