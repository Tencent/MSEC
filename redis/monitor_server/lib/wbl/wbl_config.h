
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



#ifndef __WBL_CONFIG_H__
#define __WBL_CONFIG_H__
/*
	配置接口类

	要求支持层次结构,层次以\分割,如conf["Main\\ListenPort"]取Main路径下的ListenPort配置值
	大小写敏感,
	a.name-value型配置:[]访问单个配置值,如果不存在,抛出异常conf_not_find
			GetPairs返回某个path下所有的name-value对,不考虑顺序
			相同路径下存在同样的配置名,取后一个

	b.domain型配置:GetDomains返回某个path下所有的domain配置项,与顺序相关

	c.GetSubPath返回某个path下所有的subpath名
*/

#include <string>
#include <map>
#include <vector>
#include <stdexcept>

namespace wbl {

struct conf_load_error: public std::runtime_error{ conf_load_error(const std::string& s):std::runtime_error(s){}};
struct conf_not_find: public std::runtime_error{ conf_not_find(const std::string& s):std::runtime_error(s){}};

/**
 * 配置类接口
 */
class CConfig
{
protected:
	CConfig(const CConfig&){}
public:
	CConfig(){}
	virtual ~CConfig(){}

public:
	/**
	 * 加载配置
	 * @throw conf_load_error when Load fail
	 */
	virtual void Load()=0;

	/**
	 * 取配置,要求支持层次结构,以'\'划分,如conf["Main\\ListenPort"]
	 * @throw conf_not_find when Load fail
	 */
	virtual const std::string& operator [](const std::string& name) const = 0;

	/**
	 * 取配置,要求支持层次结构,以'\'划分,如conf["Main\\ListenPort"]
	 * @return 如果配置不存在,返回""
	 */
	virtual const std::string& getvalue(const std::string& name,const std::string& defaultvalue="") const = 0;

	/**
	 * 取path下所有name-value对
	 */
	virtual const std::map<std::string,std::string>& GetPairs(const std::string& path) const = 0;
	/**
	 * 取path下所有name-value或single配置
	 */
	virtual const std::vector<std::string>& GetDomains(const std::string& path) const = 0;
	/**
	 * 取path下所有subpath名(只取一层)
	 */
	virtual const std::vector<std::string>& GetSubPath(const std::string& path) const = 0;
};

}
#endif //


