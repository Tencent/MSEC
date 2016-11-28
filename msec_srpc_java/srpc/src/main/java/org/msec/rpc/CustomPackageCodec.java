
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


package org.msec.rpc;

import org.jboss.netty.buffer.ChannelBufferOutputStream;

import java.io.IOException;


public interface CustomPackageCodec extends CustomPackageLengthChecker {

    //编码请求包：sequence需要包含到请求包中，以便请求包和响应包的一一对应
    //Return value:
    //ret == 0: succ
    //ret != 0: failed
    int encode(long sequence, ChannelBufferOutputStream stream) throws IOException;

    //获取响应包中的Sequence
    //Return value: sequence in response
    long decodeSequence(byte[] data);
}
