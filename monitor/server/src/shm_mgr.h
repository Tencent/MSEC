
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


#ifndef __SHM_MGR_H__
#define __SHM_MGR_H__

#include "multi_hash_table.h"
#include "tce_lock.h"
#include <set> 

const static size_t value_fixed_len = 128*2 + 1440*4;

class CShmMgr
{
public:
	int Init(MhtInitParam& param, bool getkey, bool& created);
	bool HasKey(string& key);
	int GetNode(string& key, char *sDataBuf, int &iDataLen, int* piVersion = NULL);
	int SetNode(string& key, string& data);
	int SetNode(string& key, const char* data, size_t size, int version);
	int	SetServiceNode(const string& real_key, string& index_key, string& data);
	void GetKeys(std::set<string>& keys_);
	int GetData(MhtIterator it, MhtData& data);
	int KeysSize() { return keys.size(); };

	const char* GetErrorMsg();
	void PrintInfo(const string& prefix);

private:
	bool GetAllServiceKeys();
	mutable tce::ReadWriteLocker locker;
	MultiHashTable ht;
	std::set<string> keys;

	friend class CDumpProcCenter;
};

#endif

