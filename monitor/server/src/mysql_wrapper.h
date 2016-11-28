
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


#ifndef __MYSQL_WRAPPER_H__
#define __MYSQL_WRAPPER_H__

#include <map>
#include <vector>
#include <string>
#include <stdexcept>
#include <string.h>
#include <mysql.h>

using namespace std;

namespace monitor {
struct MySqlRowData;
struct MySqlBasicData;
struct MySqlData;

/////////////////////////////////////////////////////////////////////////////////////////
// outside class 
// 
struct mysql_slopover: public logic_error { mysql_slopover(const string& s);};
struct mysql_execfail: public runtime_error { mysql_execfail(const string& s);};

/**
 * mysql 封装类
 * 简化使用,提供[]方式方式,自动释放资源
 */
class CMySql {
public:
	/**
	 * @throw mysql_execfail when any error happened
	 * @param host dbhost
	 * @param user 用户名
	 * @param pass 密码
     * @param port 端口
     × @param charSet 字符集, 如果为""则表示使用默认或setCharSet设置的字符集
	 */
	void Init(const string &host,const string &user,const string &pass, unsigned short port = 3306, const string& charSet="") 
		throw(mysql_execfail);
	/**
	 * @param db 等同与sql: use db
	 */
	void use(const string &db);
	/**
	 * 执行查询语句
	 * @throw mysql_execfail when any error happened
	 * @param sql sql语句
	 * @return MySqlData结构
	 * @see MySqlData
	 */
	MySqlData query(const string &sql) 
		throw(mysql_execfail);
	MYSQL* mysql(){return _Mysql;}
	void setCharSet(const string& cs) throw(mysql_execfail);

	std::string escape_string(const char * s, size_t length);
	std::string escape_string(const char *s) { return escape_string(s, strlen(s)); }
	std::string escape_string(const std::string& s) { return escape_string(s.data(), s.size()); }
public:
	CMySql();
	/**
	 * no implementation
	 */
	CMySql(const CMySql&);
	/**
	 * no implementation
	 */
	void operator = (const CMySql&); 
	~CMySql();

protected:
	void Connect();
	void Close();
	void Select();
protected:
	MYSQL * _Mysql;

	bool _bIsConn;
	string _dbname;
	string _host;
	string _user;
	string _pass;
    unsigned short _port;
	string _charSet;
};

/**
 * CMySql返回的数据集类<br>
 */
struct MySqlData {
	/**
	 * @return update/delete/insert所影响的行数
	 */
	size_t affected_rows() const;
	/**
	 * @return insert自增返回的id
	 */
	size_t auto_id() const;
	/**
	 * @return 数据集的行数
	 */
	size_t num_rows() const;
	/**
	 * @return 数据集的字段数
	 */
	size_t num_fields() const;
	/**
	 * @return 数据集的表名(2007-01-24,因mysql版本混乱,不再支持此接口)
	 */
	string org_name() const;
	/**
	 * @return 数据集的字段
	 */
	const vector<string>& Fields() const;
	/**
	 * 通过[]定位一行数据
	 * @throw mysql_slopover row超出数量
	 * @return MySqlRowData 一行数据
	 * @see MySqlRowData
	 */
	const MySqlRowData& operator [](const size_t row) const 
		throw(mysql_slopover);

public:
	/**
	 * no implementation,不能直接构造
	 */
	//MySqlData(); 
	MySqlData(MySqlBasicData* data);
	MySqlData(const MySqlData& right);
	void operator = (const MySqlData& right);
	~MySqlData();

protected:
	MySqlBasicData* _data;
};

/////////////////////////////////////////////////////////////////////////////////////////
// inside class 
// 
/**
 * CMySql返回的数据集类中的一行<br>
 */
struct MySqlRowData {
public:
	~MySqlRowData(){}
public:
	/**
	 * 通过["string"]定位到一列数据
	 * @throw mysql_slopover 数据集中不存在该字段
	 * @return string 数据值,如果为NULL，返回string("NULL")
	 * @see MySqlData
	 */
	const string& operator [](const string &s) const 
		throw(mysql_slopover);

public:
	MySqlRowData(const MySqlRowData& right);
	MySqlRowData(const vector<string>& data,map<string,int >& s2n);
	void operator=(const MySqlRowData& right);
	/**
	 * no implementation,不能直接构造
	 */
	//MySqlRowData(); 

protected:
	const vector<string>* _data;
	map<string,int >* _s2n;
};

/**
 * 内部使用的结构,不能直接操作
 */
struct MySqlBasicData {
public: //don't modify these
	vector<vector<string > > _data;
	vector<string > _col;
	map<string,int > _s2n;// fields - indx
	string _org_name;
	size_t _affected_rows;
	size_t _auto_id;

protected:
friend class MySqlData;
	vector <MySqlRowData> _rows;
	size_t _nRefCount;
	void RefAdd();
	void RefSub();

public:
	MySqlBasicData();
	MySqlBasicData(const MySqlBasicData&);// no implementation
	~MySqlBasicData(){}//{cerr << "delete " << this << endl;}

	/**
	 * 返回影响的行数
	 */
	size_t affected_rows() const
		{return _affected_rows;}
	/**
	 * 返回insert中自增的id
	 */
	size_t auto_id() const
		{return _auto_id;}
	/**
	 * 返回结果集行数
	 */
	size_t num_rows() const 
		{return _data.size();}
	/**
	 * 返回字段数
	 */
	size_t num_fields() const
		{return _col.size();}
	/**
	 * 返回表名
	 */
	string org_name() const
		{return _org_name;}
	/**
	 * 返回字段
	 */
	const vector<string>& Fields() const
		{return _col;}

	/**
	 * 设置表名
	 */
	void org_name(const string& s) 
		{_org_name = s;}
	/**
	 * 设置影响行数
	 */
	void affected_rows(const size_t i) 
		{_affected_rows = i;}
	/**
	 * 设置自增id
	 */
	void auto_id(const size_t i) 
		{_auto_id = i;}
	/**
	 * 设置字段
	 * @throw mysql_slopover 字段名重复
	 */
	void Fields(const vector<string>& v) 
		throw(mysql_slopover);
	/**
	 * 清除数据
	 */
	void clear() {_data.clear();}
	void push_back(vector<string>& v) 
		throw(mysql_slopover);
	void genrows();
};


}

#endif //


