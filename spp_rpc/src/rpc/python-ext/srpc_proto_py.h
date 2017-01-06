
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


#ifndef __SRPC_PROTO_PY_H__
#define __SRPC_PROTO_PY_H__


#include <stdint.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int srpc_py_set_header(void *header);
void srpc_py_set_service_name(const char *sname, int len);
int srpc_py_pack(const char *method_name, uint64_t seq, const void *body, int body_len, char *buf, int32_t *plen);
int srpc_py_unpack(const char *buf, int buf_len, char *body, int *plen, uint64_t *seq, int32_t *err);
int srpc_py_check_pkg_len(void *buff, int len);
const char *srpc_py_err_msg(int err);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

