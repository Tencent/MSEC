
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

import com.google.protobuf.ByteString;
import com.google.protobuf.MessageLite;
import org.msec.net.NettyCodecUtils;
import srpc.Head;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.net.Socket;


public class SrpcProxy {
    RpcRequest  rpcRequest = new RpcRequest();
    String  caller;
    int errno;
    String errmsg;
    long sequence;

    public void setMethod(String method) {
        rpcRequest.setMethodName(method);
    }

    public void setSequence(long seq) {
        rpcRequest.setSeq(seq);
    }

    public void setCaller(String caller) {
        this.caller = caller;
    }

    public int getErrno() {
        return errno;
    }

    public String getErrmsg() {
        return errmsg;
    }

    public long getSequence() {
        return sequence;
    }

    public byte[] serialize(MessageLite request) {
        Head.CRpcHead.Builder builder = Head.CRpcHead.newBuilder();
        String serviceMethodName;
        if (rpcRequest.getServiceName() == null || rpcRequest.getServiceName().isEmpty()) {
            serviceMethodName = rpcRequest.getMethodName();
        } else {
            serviceMethodName = rpcRequest.getServiceName() + "." + rpcRequest.getMethodName();
        }
        builder.clear();
        builder.setSequence(rpcRequest.getSeq());
        builder.setColoring(0);
        builder.setColorId(NettyCodecUtils.generateColorId(rpcRequest.getMethodName()));
        builder.setFlowId(rpcRequest.getSeq() ^ builder.getColorId());

        builder.setErr(0);
        builder.setResult(0);
        builder.setErrMsg(ByteString.EMPTY);
        builder.setCaller(ByteString.copyFromUtf8(caller));
        builder.setMethodName(ByteString.copyFrom((serviceMethodName).getBytes()));
        builder.addCallerStack(ByteString.EMPTY);

        Head.CRpcHead rpcHead = builder.build();
        int rpcHeadLen = rpcHead.getSerializedSize();
        int rpcBodyLen = request.getSerializedSize();
        int totalLen = 1 + 4 + 4 + rpcHeadLen + rpcBodyLen + 1;
        byte[] buffer = new byte[totalLen];

        buffer[0] = '(';
        NettyCodecUtils.int32_to_buf(buffer, 1, rpcHeadLen);
        NettyCodecUtils.int32_to_buf(buffer, 5, rpcBodyLen);
        System.arraycopy(rpcHead.toByteArray(), 0, buffer, 5 + 4, rpcHeadLen);
        System.arraycopy(request.toByteArray(), 0, buffer, 5 + 4 + rpcHeadLen, rpcBodyLen);
        buffer[totalLen - 1] = ')';
        return buffer;
    }

    //检查响应包的长度
    //Return value:
    //ret > 0: package length
    //ret == 0: package not complete
    //ret < 0: wrong package format
    public static int checkPackage(byte[] data, int length) {
        if (length < 10) {
            return 0;
        }

        byte  stx = data[0];
        if (stx != (byte)'(') {
            return -1;
        }

        int headLength = NettyCodecUtils.buf_to_int32(data, 1);
        int bodyLength = NettyCodecUtils.buf_to_int32(data, 5);
        int totalLength = 10 + headLength + bodyLength;
        if (length < totalLength) {
            return 0;
        }
        return totalLength;
    }

    public MessageLite deserialize(byte[] data, int length, MessageLite responseInstance) throws  Exception {

        if (checkPackage(data, length) != length) {
            throw new Exception("Invalid data: maybe not complete or wrong format!");
        }
        int headLength = NettyCodecUtils.buf_to_int32(data, 1);
        int bodyLength = NettyCodecUtils.buf_to_int32(data, 5);
        byte[]  headBytes = new byte[headLength];
        byte[]  bodyBytes = new byte[bodyLength];
        System.arraycopy(data, 9, headBytes, 0, headLength);
        System.arraycopy(data, 9 + headLength, bodyBytes, 0, bodyLength);

        Head.CRpcHead pbHead = Head.CRpcHead.parseFrom(headBytes);

        errno = pbHead.getErr();
        sequence = pbHead.getSequence();
        if (pbHead.getErrMsg() != null && !pbHead.getErrMsg().isEmpty()) {
            errmsg = pbHead.getErrMsg().toStringUtf8();
        } else {
            errmsg = "";
        }

        if (errno == 0) {
            return responseInstance.getParserForType().parseFrom(bodyBytes);
        }
        return null;
    }
}
