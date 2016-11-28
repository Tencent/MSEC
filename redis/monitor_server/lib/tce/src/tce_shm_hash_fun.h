
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


#ifndef __TCE_SHM_HASH_FUN__
#define __TCE_SHM_HASH_FUN__

#include "tce_def.h"

namespace tce{
	namespace shm{


template <class _Key> struct hash { };

inline size_t __tce_hash_string(const char* __s)
{
	size_t __h = 0; 
	for ( ; *__s; ++__s)
		__h = 5*__h + *__s;

	return size_t(__h);
}

inline size_t __tce_hash_string(const std::string& __s)
{
	size_t __h = 0; 
	for ( size_t i=0; i<__s.size(); ++i )
		__h = 5*__h + __s[i];

	return size_t(__h);
}

__TCE_TEMPLATE_NULL struct hash<char*>
{
	size_t operator()(const char* __s) const { return __tce_hash_string(__s); }
};

__TCE_TEMPLATE_NULL struct hash<const char*>
{
	size_t operator()(const char* __s) const { return __tce_hash_string(__s); }
};


__TCE_TEMPLATE_NULL struct hash<std::string>
{
	size_t operator()(const std::string& __s) const { return __tce_hash_string(__s); }
};

__TCE_TEMPLATE_NULL struct hash<const std::string>
{
	size_t operator()(const std::string __s) const { return __tce_hash_string(__s); }
};

__TCE_TEMPLATE_NULL struct hash<char> {
	size_t operator()(char __x) const { return __x; }
};
__TCE_TEMPLATE_NULL struct hash<unsigned char> {
	size_t operator()(unsigned char __x) const { return __x; }
};
__TCE_TEMPLATE_NULL struct hash<signed char> {
	size_t operator()(unsigned char __x) const { return __x; }
};
__TCE_TEMPLATE_NULL struct hash<int16_t> {
	size_t operator()(int16_t __x) const { return __x; }
};
__TCE_TEMPLATE_NULL struct hash<uint16_t> {
	size_t operator()(uint16_t __x) const { return __x; }
};
__TCE_TEMPLATE_NULL struct hash<int32_t> {
	size_t operator()(int32_t __x) const { return __x; }
};
__TCE_TEMPLATE_NULL struct hash<uint32_t> {
	size_t operator()(uint32_t __x) const { return __x; }
};
// __TCE_TEMPLATE_NULL struct hash<long> {
	// size_t operator()(long __x) const { return __x; }
// };
// __TCE_TEMPLATE_NULL struct hash<unsigned long> {
	// size_t operator()(unsigned long __x) const { return __x; }
// };

__TCE_TEMPLATE_NULL struct hash<int64_t> {
	int64_t operator()(int64_t __x) const { return __x; }
};

__TCE_TEMPLATE_NULL struct hash<uint64_t> {
	uint64_t operator()(uint64_t __x) const { return __x; }
};

	};
};

#endif

