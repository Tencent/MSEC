
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


#ifndef _NOTIFY_H_
#define _NOTIFY_H_

#include <stddef.h>

extern int g_spp_shm_fifo;

namespace tbase {
    namespace notify {
        //用于通知共享内存有数据可以接收
        class CNotify
        {
        public:
            /**
             * @brief  获取fifo创建的路径
             * @key    key信息
             * @return 返回fifo创建的路径
             */
            static char *get_fifo_path(int key);

            /**
             * @brief 通知管道初始化
             *
             * @param key 传入参数，FIFO根据32位key唯一生成
             * @param fifoname 传出参数，FIFO文件名
             * @param fifoname_size 传入参数，FIFO文件名buffer size
             *
             * @return >0 成功; -1 key非法; -2 mkfifo错误; -3 open错误
             */
            static int notify_init(int key, char *fifoname = NULL, size_t fifoname_size = 0);

            /**
             * @brief 发送通知
             *
             * @param fd FIFO的fd
             *
             * @return >0 写字节数; 0 没有可写数据; <0 写错误
             */
            static int notify_send(int fd);

            /**
             * @brief 批量接收通知
             *
             * @param fd FIFO的fd
             *
             * @return >0 读到字节数; 0 没有可读数据; <0 读错误
             */
            static int notify_recv(int fd);
        };
    }
}
#endif
