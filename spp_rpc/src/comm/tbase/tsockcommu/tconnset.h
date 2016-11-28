
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


#ifndef _TBASE_TSOCKCOMMU_CONNSET_H_
#define _TBASE_TSOCKCOMMU_CONNSET_H_
#include <time.h>
#include <list>
#include "tcache.h"
#include "tmempool.h"

#define  E_NEED_CLOSE  10000
#define  E_NOT_FINDFD  20000
#define  C_NEED_SEND   30000
#define  E_RECV_DONE   40000

//bug fix 3350521, by aoxu, 2010-03-09
#define  MAX_FLOW_NUM  0x7FFFFFFF


namespace tbase
{
    namespace tcommu
    {
        namespace tsockcommu
        {
			//连接池集合
            class CConnSet
            {
            public:
                CConnSet(CMemPool& mp, int maxconn);
                ~CConnSet();

                //打印flow所连接的信息
                void dumpinfo(unsigned flow, int log_type, const char* errmsg);
				//添加连接
                int addconn(unsigned& flow, int fd, int type);
				//查询fd
                int fd(unsigned flow);

				//接收数据
                int recv(unsigned flow);
				//发送数据
                int send(unsigned flow, const char* data, size_t data_len);
				//从缓冲区接收数据
                int sendfromcache(unsigned flow);

				//关闭连接
                int closeconn(unsigned flow);
				//检查连接是否过期
                void check_expire(time_t access_deadline, std::list<unsigned>& timeout_flow);
                //检查单个flow的连接是否过期，只在EPOLLIN中使用。
                int check_per_expire(unsigned flow, time_t access_deadline);

                //获取当前使用的flow个数
                int getusedflow();

				//查找连接对应的cache
                inline ConnCache* getcc(unsigned flow) {
                    return ccs_[flow % maxconn_];
                }
            private:
                ConnCache** ccs_;	//连接cache集合
                int maxconn_;		//建立的最大连接数
                int usedflow_;      //当前使用的flow
            };

        }
    }
}
#endif
