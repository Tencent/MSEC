
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



#ifndef __CONFIGINI_C_H__
#define __CONFIGINI_C_H__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * @brief  获取INI配置C接口
 * @return NULL     获取失败，没有该配置
 * @       !=NULL   获取配置成功，调用者负责释放内存
 */
char *GetConfig(const char *filename, const char *session, const char *key);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

