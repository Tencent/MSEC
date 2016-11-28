
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


#ifndef _BENCHAPIPLUS_H_
#define _BENCHAPIPLUS_H_

typedef int  (*spp_handle_init_t)(void*, void*);
typedef int  (*spp_handle_close_t)(unsigned, void*, void*);
typedef int  (*spp_handle_input_t)(unsigned, void*, void*);
typedef int  (*spp_handle_route_t)(unsigned, void*, void*);
typedef int  (*spp_handle_process_t)(unsigned, void*, void*);
typedef int  (*spp_handle_exception_t)(unsigned, void*, void*);
typedef void (*spp_handle_fini_t)(void*, void*);
typedef void (*spp_handle_loop_t)(void*);
typedef bool (*spp_mirco_thread_t)(void);
typedef void (*spp_handle_switch_t)(bool);
typedef void (*spp_set_notify_t)(int);

typedef struct {
    void *handle;
    spp_handle_init_t       spp_handle_init;
    spp_handle_close_t      spp_handle_close;
    spp_handle_input_t      spp_handle_input;
    spp_handle_route_t      spp_handle_route;
    spp_handle_process_t    spp_handle_process;
    spp_handle_exception_t  spp_handle_exception;
    spp_handle_fini_t       spp_handle_fini;
    spp_handle_loop_t       spp_handle_loop;
    spp_mirco_thread_t      spp_mirco_thread;
    spp_handle_switch_t     spp_handle_switch;
    spp_set_notify_t        spp_set_notify;
} spp_dll_func_t;
extern spp_dll_func_t sppdll;

#endif
