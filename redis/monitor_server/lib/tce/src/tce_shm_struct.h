
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


#ifndef __TCE_SHM_STRUCT_H__
#define __TCE_SHM_STRUCT_H__

#include "tce_shm.h"
#include <stdexcept>


namespace tce{

namespace shm{

	template <class _Tp>
	class CStruct
	{
		typedef _Tp value_type;
		typedef size_t size_type;
	public:
		CStruct()
			:m_bInit(false)
			,m_pValue(NULL)
		{

		}
		~CStruct(){

		}
		bool init(const int32_t iShmKey, const bool bReadOnly=false){
			if ( m_bInit )
				return true;

			size_type nShmMaxSize = sizeof(value_type);
			if ( !m_oShm.Init( iShmKey, nShmMaxSize ) )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"struct Init Failed:%s", m_oShm.GetErrMsg());
				return false;
			}

			//根据是否只读方式绑定共享内存
			if ( !m_oShm.Attach(bReadOnly) )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"struct Init Failed:%s", m_oShm.GetErrMsg());
				return false;
			}

			m_pValue = (value_type*)reinterpret_cast<char*>(m_oShm.GetShmBuf());
			
			m_bInit = true;
			return true;



		}

		bool IsCreate() const {	return m_oShm.IsCreate();	}

		value_type& value(){
			return *m_pValue;
		}
		const value_type& value() const {
			return *m_pValue;
		}
		const char* GetErrMsg() const {	return m_szErrMsg;	}

	private:
		bool m_bInit;
		value_type* m_pValue;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
	};

};

};


#endif

