
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



#ifndef __SRPC_CINTF_H__
#define __SRPC_CINTF_H__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void set_log_option_str(const char *key, const char *val);
void set_log_option_long(const char *key, int64_t val);
void reset_log_option(void);
void msec_log_error(const char *str);
void msec_log_info(const char *str);
void msec_log_debug(const char *str);
void msec_log_fatal(const char *str);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

