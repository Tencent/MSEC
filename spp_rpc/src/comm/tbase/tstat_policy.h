
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


#ifndef _TBASE_TSTAT_POLICY_H_
#define _TBASE_TSTAT_POLICY_H_

//#include <asm/atomic.h>
#include "atomic.h"
#include <stdio.h>

#define STAT_ID_MAX_LEN   64			//统计ID字符串最大长度
//edit by aoxu, 2010-02-10
//为保证统计文件格式的平台无关性，原子数全部使用32位的atomic_t，保证32/64位下长度一致
/*
#ifdef __x86_64__						//64位
#define TStatVal 				atomic64_t
#define STATVAL_INC(val)		atomic64_inc(&(val))
#define STATVAL_DEC(val)		atomic64_dec(&(val))
#define STATVAL_ADD(val, lv)	atomic64_add(lv, &(val))
#define STATVAL_SET(val, lv)	atomic64_set(&(val), lv)
#define STATVAL_READ(val)		atomic64_read(&(val))
#else									//32位
*/
#define TStatVal 				atomic_t
#define STATVAL_INC(val)		atomic_inc(&(val))
#define STATVAL_DEC(val)		atomic_dec(&(val))
#define STATVAL_ADD(val, lv)	atomic_add(lv, &(val))
#define STATVAL_SET(val, lv)	atomic_set(&(val), lv)
#define STATVAL_READ(val)		atomic_read(&(val))
//#endif

namespace tbase
{
    namespace tstat
    {
        ///////////////////////////////////////////////////////////////////////////////////
        typedef struct {
            char* id_;							//统计ID
            char* desc_;                        //统计描述
            int type_;							//统计类型
            int val_size_;                      //统计值维数
            TStatVal* count_;					//次数
            TStatVal* value_;					//统计值
        } TStatObjWrapper; //外部可见的统计对象

        ///////////////////////////////////////////////////////////////////////////////////
        //统计策略
        class CTStatPolicy
        {
        public:
            CTStatPolicy() {}
            virtual ~CTStatPolicy() {}
            //一次统计
            //target:  统计对象
            //val:	   统计值分量
            //val_idx: 统计值维度
            virtual void __step__(TStatObjWrapper* target, long val, int val_idx) = 0;
            //取统计结果
            //target:  统计对象
            //val:	   统计值
            //val_size: 统计值维数
            //返回值:   统计次数
            virtual long __result__(TStatObjWrapper* target, long* val, long* val_size) {
                //assert(target != NULL && target->val_size_ > 0 && val != NULL);
                if (target == NULL || target->val_size_ <= 0 || val == NULL) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target val_size_:%d should be > 0\n"
                           "* val should not be NULL\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                *val_size = target->val_size_;

                for (int i = 0; i < *val_size; ++i)
                    val[i] = STATVAL_READ(target->value_[i]);

                return STATVAL_READ(*(target->count_));;
            }
            //重置统计值
            virtual void __reset__(TStatObjWrapper* target) {
                //assert(target != NULL && target->val_size_ > 0);
                if (target == NULL || target->val_size_ <= 0) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target val_size_:%d should > 0\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                for (int i = 0; i < target->val_size_; ++i)
                    STATVAL_SET(target->value_[i], 0);

                STATVAL_SET(*(target->count_), 0);
            }
            //统计策略名
            virtual const char* __tag__() = 0;
        };

        //add by jeremy 设置操作
        class CTStatPolicySet : public CTStatPolicy
        {
        public:
            CTStatPolicySet() {}
            ~CTStatPolicySet() {}
            virtual void __step__(TStatObjWrapper* target, long val, int val_idx) {
                if (target == NULL || target->val_size_ <= 0) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target val_size_:%d should be > 0\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                STATVAL_SET(target->value_[val_idx - 1], val);

                if (val_idx == target->val_size_)
                    STATVAL_INC(*(target->count_));
            }
            virtual const char* __tag__() {
                return "Set";
            }
        };

        //累加操作
        class CTStatPolicySum : public CTStatPolicy
        {
        public:
            CTStatPolicySum() {}
            ~CTStatPolicySum() {}
            virtual void __step__(TStatObjWrapper* target, long val, int val_idx) {
                //assert(target != NULL && target->val_size_ > 0);
                if (target == NULL || target->val_size_ <= 0) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target val_size_:%d should be > 0\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                STATVAL_ADD(target->value_[val_idx - 1], val);

                if (val_idx == target->val_size_)
                    STATVAL_INC(*(target->count_));
            }
            virtual const char* __tag__() {
                return "Sum";
            }
        };

        //平均操作
        class CTStatPolicyAvg : public CTStatPolicy
        {
        public:
            CTStatPolicyAvg() {}
            ~CTStatPolicyAvg() {}
            virtual void __step__(TStatObjWrapper* target, long val, int val_idx) {
                //assert(target != NULL && target->val_size_ > 0);
                if (target == NULL || target->val_size_ <= 0) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target val_size_:%d should be > 0\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                STATVAL_ADD(target->value_[val_idx - 1], val);

                if (val_idx == target->val_size_)
                    STATVAL_INC(*(target->count_));
            }
            virtual long __result__(TStatObjWrapper* target, long* val, long* val_size) {
                //assert(target != NULL && target->val_size_ > 0 && val != NULL);
                if (target == NULL || target->val_size_ <= 0 || val == NULL) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target->val_size_:%d should be > 0\n"
                           "* val should not be NULL\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                long temp = STATVAL_READ(*(target->count_));

                if (temp > 0) {
                    *val_size = target->val_size_;

                    for (int i = 0; i < *val_size; ++i)
                        val[i] = STATVAL_READ(target->value_[i]) / temp;

                    return temp;
                } else {
                    return 0;
                }
            }
            virtual const char* __tag__() {
                return "Avg";
            }
        };

        //最大值操作
        class CTStatPolicyMax : public CTStatPolicy
        {
        public:
            CTStatPolicyMax() {}
            ~CTStatPolicyMax() {}
            virtual void __step__(TStatObjWrapper* target, long val, int val_idx) {
                //assert(target != NULL && target->val_size_ > 0);
                if (target == NULL || target->val_size_ <= 0) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target val_size_:%d should be > 0\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                long temp = STATVAL_READ(target->value_[val_idx - 1]);

                if (temp < val)
                    STATVAL_SET(target->value_[val_idx - 1], val);

                if (val_idx == target->val_size_)
                    STATVAL_INC(*(target->count_));
            }
            virtual const char* __tag__() {
                return "Max";
            }
        };

        //最小值操作
        class CTStatPolicyMin : public CTStatPolicy
        {
        public:
            CTStatPolicyMin() {}
            ~CTStatPolicyMin() {}
            virtual void __step__(TStatObjWrapper* target, long val, int val_idx) {
                //assert(target != NULL && target->val_size_ > 0);
                if (target == NULL || target->val_size_ <= 0) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target val_size_:%d should be > 0\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                long temp = STATVAL_READ(target->value_[0]);

                if (temp > val || temp == 0) //fixed me! defect!
                    STATVAL_SET(target->value_[0], val);

                STATVAL_INC(*(target->count_));
            }
            virtual const char* __tag__() {
                return "Min";
            }
        };

        //计数操作
        class CTStatPolicyCount : public CTStatPolicy
        {
        public:
            CTStatPolicyCount() {}
            ~CTStatPolicyCount() {}
            virtual void __step__(TStatObjWrapper* target, long val, int val_idx) {
                //assert(target != NULL && target->val_size_ > 0);
                if (target == NULL || target->val_size_ <= 0) {
                    printf("[ERROR]Please check:\n"
                           "* target should not be NULL\n"
                           "* target val_size_:%d should be > 0\n",
                           target->val_size_
                          );
                    exit(-1);
                }

                STATVAL_ADD(target->value_[val_idx - 1], val);

                if (val_idx == target->val_size_)
                    STATVAL_INC(*(target->count_));
            }
            virtual const char* __tag__() {
                return "Count";
            }
        };
        //...更多统计策略待扩展...
    }
}
#endif
