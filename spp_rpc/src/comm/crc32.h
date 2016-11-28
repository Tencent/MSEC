
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


#ifndef _CIPHER_CRC32_H_
#define _CIPHER_CRC32_H_

#include <stdint.h>

namespace platform
{
namespace commlib
{
class CCrc32
{
public:
    CCrc32() : crc(0) {}

    /*
     *@brief 增量计算crc32的值
     *@ptr 待计算的数据
     *@len 待计算的数据长度
     *@return  = 0  成功
               < 0  失败
     */
    int Update(unsigned char *ptr, uint32_t len);
    /*
     *@brief 将Update的增量计算结果返回
     *@return  crc32的结果
     */
    uint32_t Final();

    /*
     *@brief 一次性计算crc32值
     *@ptr 待计算的数据
     *@len 待计算的数据长度
     *@return  crc32的结果
     */
    uint32_t Crc32(unsigned char *ptr, uint32_t len);

private:
    uint32_t crc;
};
}
}

#endif /* _CIPHER_CRC32_H_ */
