
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


#ifndef __TCE_ERRCODE_H__#define __TCE_ERRCODE_H__enum TCE_ERRCODE{	TEC_OK = 0,		//正确	TEC_PARAM_ERROR = 1, //参数错误	TEC_SYSTEM_ERROR = 2, //系统错误，不应该发生	TEC_SOCKET_BUFFER_FULL = 3,	//socket buffer full，当发生，底层会关闭改链接	TEC_SOCKET_SEND_ERROR = 4,	TEC_BUFFER_ERROR = 5,	TEC_DATA_ERROR = 6,	TEC_SOCKET_FULL = 7,	TEC_CONNECT_ERROR = 8,	TEC_UNKOWN_ERROR = 100, //未知错误};#endif