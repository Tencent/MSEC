
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


/*
 * =====================================================================================
 *
 *       Filename:  myatomic_gcc8.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  07/07/2009 05:38:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#ifndef _MYATOMIC_GCC8_h
#define _MYATOMIC_GCC8_h

#define atomic64_t atomic8_t
#define atomic64_read atomic8_read
#define atomic64_set atomic8_set
#define atomic64_inc atomic8_inc
#define atomic64_add atomic8_add
#endif
