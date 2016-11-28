
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


#ifndef _TBASE_TSOCKCOMMU_TCACHE_H_
#define _TBASE_TSOCKCOMMU_TCACHE_H_
#include "tmempool.h"
#include "tsocket.h"
#include "tcommu.h"
#include <sys/time.h>

namespace tbase
{
    namespace tcommu
    {
        namespace tsockcommu
        {
            //纯cache对象
            class CRawCache
            {
            public:
                CRawCache(CMemPool& mp);
                ~CRawCache();

                char* data();
                unsigned data_len();
                void append(const char* data, unsigned data_len);
                void skip(unsigned length);

                void set_fin_bit(bool val) {
                    _fin_bit = val;
                }
                bool get_fin_bit() {
                    return _fin_bit;
                }

            private:
                //内存池对象
                CMemPool& _mp;
                //内存基址
                char* _mem;
                //内存大小
                unsigned _block_size;
                //实际使用内存起始偏移量
                unsigned _data_head;
                //实际使用内存长度
                unsigned _data_len;
                //设置flush and close标志位, fixed send 0 shuffle bug.
                bool _fin_bit;
            };

            //连接对象cache
            class ConnCache
            {
            public:
                ConnCache(CMemPool& mp) : _flow(0), _fd(0), _type(0), _r(mp), _w(mp)
				{
                    _access.tv_sec = 0;					
                    _access.tv_usec = 0;
				}
                ~ConnCache() {}

                //连接唯一标示
                unsigned _flow;
                //相关fd
                int _fd;
                //时间戳
                struct timeval _access;
                //连接类型: TCP_SOCKET\UDP_SOCKET\UNIX_SOCKET
                int _type;
                //对端信息:
                CSocketAddr _addr;
                //socket信息
                TConnExtInfo _info;
                //读请求cache
                CRawCache _r;
                //写回复cache
                CRawCache _w;
            };
        }
    }
}
#endif
