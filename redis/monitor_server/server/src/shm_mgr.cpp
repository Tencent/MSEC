
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


#include "shm_mgr.h"
#include "data.pb.h"

int CShmMgr::Init(MhtInitParam& param, bool getkey, bool&  created)
{
	created = false;
	int ret = ht.InitFromShm(param.ddwShmKey);
	if (ret == 0)
	{
		if(getkey && !GetAllServiceKeys())
			return -10;
		return ret;
	}

	ret = ht.CreateFromShm(param);
	if (ret != 0) return ret;
	created = true;
	return  0;
}

bool CShmMgr::HasKey(string& key)
{
	tce::ReadLocker rl(locker);
	
	return	ht.HasKey(key.data(), key.size());
}

int CShmMgr::GetNode(string& key, char *sDataBuf, int &iDataLen, int *piVersion)
{
	tce::ReadLocker rl(locker);
	
	return	ht.GetData(key.data(), key.size(), sDataBuf, iDataLen, piVersion);
}

int CShmMgr::SetNode(string& key, string& data)
{
	tce::WriteLocker wl(locker);
	return ht.SetData(key.data(), key.size(), data.data(), data.size(), -1, true);
}

int CShmMgr::GetData(MhtIterator it, MhtData& data)
{
	tce::ReadLocker rl(locker);
	
	return	ht.Get(it, data);
}

int CShmMgr::SetServiceNode(const string& real_key, string& index_key, string& data)
{
	tce::WriteLocker wl(locker);
	int ret = ht.SetData(index_key.data(), index_key.size(), data.data(), data.size(), -1, true);
	if( ret == 0 && !real_key.empty()) {
		keys.insert(real_key);
	}
	return ret;
}

int CShmMgr::SetNode(string& key, const char* data, size_t size, int version)
{
	tce::WriteLocker wl(locker);
	return ht.SetData(key.data(), key.size(), data, size, version, true);
}

bool CShmMgr::GetAllServiceKeys()
{
	keys.clear();
	MhtIterator it;
	MhtData* data = new MhtData();
	int ret = 0;
	for (it = ht.Begin(); it != ht.End(); it = ht.Next(it))
	{
		ret = ht.Get(it, *data);
		if (ret != 0)
		{
	        delete data;
			return false;
		}
		if(data->klen == 16 && data->dlen > 0) {
			msec::monitor::data::ServiceData servicedata;		//Service - ÊôÐÔÊý¾Ý

			if(servicedata.ParseFromArray(data->data, data->dlen))
			{
				keys.insert(servicedata.servicename());
			}
		}
	}
	delete data;
	return	true;
}

void CShmMgr::GetKeys(set<string>& keys_) {	
	tce::ReadLocker wl(locker);
	keys_ = keys;
}

const char* CShmMgr::GetErrorMsg()
{
	return ht.GetErrorMsg();
}

void CShmMgr::PrintInfo(const string& prefix)
{
	tce::ReadLocker rl(locker);
	//ht.PrintInfo(prefix);
}
