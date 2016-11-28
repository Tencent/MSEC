
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


#ifndef _TBASE_TSOCKCOMMU_H_
#define _TBASE_TSOCKCOMMU_H_
#include "atomic.h"
#include <time.h>
#include <vector>
#include <map>
#include "../tcommu.h"

using namespace tbase::tcommu;

namespace tbase
{
namespace tcommu
{
namespace tsockcommu
{
typedef struct
{
    unsigned int ip_;			//ip address
    unsigned short port_;		//ip port
} TIpport;

typedef struct
{
    int type_;					//tcp or udp or unix socket or notify fd
    bool isabstract_;			//is abstract namespace unix domain socket
    int TOS_;                   //-1:off, >=0:set socket TOS value
    int oob_;
    union {
        TIpport ipport_;        //ip and port
        char path_[256];		//unix socket path
    };
} TSockBind;

/*typedef struct {
	int fd_;					//fd
	int type_;					//fd type
	unsigned localip_;			//local ip
	unsigned short localport_;	//local port
	unsigned remoteip_;			//remote ip
	unsigned short remoteport_;	//remote port
}TConnExtInfo;
*/
typedef struct
{
    std::vector<TSockBind> sockbind_; //绑定信息
    int maxconn_;		//最大连接数 > 0
    int maxpkg_; 		//最大包量，如果为0则不检查
    int expiretime_;	//超时时间， > 0
    bool udpautoclose_;	//UDP回包后是否自动关闭flow
    unsigned sendcache_limit_;  // 发包cache最大值
} TSockCommuConf;

//必须注册CB_RECVDATA回调函数
class CTSockCommu : public CTCommu
{
public:
    CTSockCommu();
    ~CTSockCommu();
    int init(const void* config);
    int addnotify(int fd);
    int clear();
    int poll(bool block = false);
    int sendto(unsigned flow, void* arg1, void* arg2);
    int ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2);

    int getconn();

    int add_notify(int fd);
    int del_notify(int fd);    

    void set_sendcache_limit(unsigned limit);
    bool check_over_sendcache_limit(void *cache);
    
protected:
    std::map<int, TSockBind> sockbind_; //绑定信息
    int maxconn_;
    int expiretime_;
    unsigned sendcache_limit_;

    time_t lastchecktime_;
    bool udpautoclose_;
    unsigned flow_;
    blob_type buff_blob_;
    TConnExtInfo extinfo_;
    struct TInternal;
    struct TInternal* ix_;

    void fini();
    void check_expire(void);
    int create_sock(const TSockBind& s);
    void handle_accept(int fd);
    void handle_accept_udp(int fd);
};
}
}
}
#endif
