
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


#include "keygen.h"
#include "crc32.h"

#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <string.h>
#include <linux/limits.h>

using namespace spp::comm;
using namespace platform::commlib;

namespace spp
{
namespace comm
{
//use pwd & id to generate shm/mq key
//if mq, id = 255
//if shm, id = groupid
key_t pwdtok(int id)
{
    char seed[PATH_MAX] = {0};
    readlink("/proc/self/exe", seed, PATH_MAX);
    dirname(seed);	//seed changed

    //把keyseed和id组合成一个字符串
    uint32_t seed_len = strlen(seed) + sizeof(id);
    memcpy(seed + strlen(seed), &id, sizeof(id));

    CCrc32 generator;
    return generator.Crc32((unsigned char*)seed, seed_len);
}
} // end namespace comm
} // end namespace spp

