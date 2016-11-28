
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


#ifndef __TCE_SINGLETON_H__
#define __TCE_SINGLETON_H__
#include <memory>
#include <cstddef>

namespace tce{

	class CNonCopyAble{
	protected:
		CNonCopyAble(){}
		virtual ~CNonCopyAble(){}
	private:
		CNonCopyAble& operator=(const CNonCopyAble& rhs);
		CNonCopyAble(const CNonCopyAble& rhs);
	};

	template <typename T>
	class CSingleton
		: private CNonCopyAble
	{
	public:
		static T& GetInstance(void)
		{
			if ( NULL == m_pInstance.get() )
			{
				m_pInstance.reset(new T);
			}

			return *m_pInstance;
		}

	//	friend class std::auto_ptr<T>;

	protected:
		CSingleton(void)	{}

		#define DECLARE_SINGLETON_CLASS(type) \
		friend class tce::CSingleton<type>;\
		static type& GetInstance(void) \
		{\
			return tce::CSingleton<type>::GetInstance();\
		}

	private:
		static std::auto_ptr<T> m_pInstance;
	};

	template <typename T>
	std::auto_ptr<T> CSingleton<T>::m_pInstance;
}
#endif

