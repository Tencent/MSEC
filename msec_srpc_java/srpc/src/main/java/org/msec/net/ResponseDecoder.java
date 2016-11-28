
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


package org.msec.net;

import org.apache.log4j.Logger;
import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.buffer.DynamicChannelBuffer;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.oneone.OneToOneDecoder;
import org.msec.rpc.CustomPackageCodec;
import org.msec.rpc.CustomPackageLengthChecker;
import org.msec.rpc.RpcResponse;
import srpc.Head;

import java.util.ArrayList;
import java.util.List;


public class ResponseDecoder extends OneToOneDecoder {
    private static Logger log = Logger.getLogger(ResponseDecoder.class.getName());

    @Override
    protected Object decode(ChannelHandlerContext channelHandlerContext, Channel channel, Object msg) throws Exception {
        long receiveTime = System.currentTimeMillis();
        if (!(msg instanceof ChannelBuffer)) {
            return msg;
        }
        //System.out.println("Response recvtime: " + System.currentTimeMillis());

        List<Object> messages = null;
        ChannelBuffer cb = (ChannelBuffer) NettyCodecUtils.getAttachment(channelHandlerContext, Constants.ATTACHMENT_BYTEBUFFER);
        ChannelBuffer cb_ = (ChannelBuffer) msg;
        if (cb == null) {
            cb = cb_;
        } else {
            cb.writeBytes(cb_);
        }

        ChannelHandlerContext  encodeCtx = channelHandlerContext.getPipeline().getContext("encoder");
        Integer protoFlag = (Integer)NettyCodecUtils.getAttachment(encodeCtx, Constants.ATTACHMENT_CUSTOM_PROTO_FLAG);
        if (protoFlag != null && protoFlag.intValue() == 1) {
            RpcResponse rpcResponse = getCustomPackage(channelHandlerContext, channel, cb, encodeCtx);
            if (rpcResponse == null) {
                return null;
            }

            if (messages == null) {
                messages = new ArrayList<Object>();
            }
            messages.add(rpcResponse);
            return messages;
        }

        int lastReadIndex = cb.readerIndex();
        while (cb.readable()) {
            if (cb.readableBytes() < Constants.PKG_LEAST_LENGTH) {
                setAttachment(channelHandlerContext, channel, cb, lastReadIndex);
                break;
            }

            byte  stx = cb.readByte();
            if (stx != (byte)'(') {
                log.error("Invalid packge stx.");
                channelHandlerContext.getChannel().close();
                throw new IllegalArgumentException("Request decode error: invalid package stx " + stx);
            }

            int headLength = cb.readInt();
            int bodyLength = cb.readInt();
            if (cb.readableBytes() < headLength + bodyLength + 1) {
                setAttachment(channelHandlerContext, channel, cb, lastReadIndex);
                break;
            }

            byte[]  headBytes = new byte[headLength];
            byte[]  bodyBytes = new byte[bodyLength];
            cb.readBytes(headBytes);
            cb.readBytes(bodyBytes);
            byte etx = cb.readByte();
            if (etx != ')') {
                log.error("Invalid package etx.");
                channelHandlerContext.getChannel().close();
                throw new IllegalArgumentException("Request decode error: invalid package etx " + etx);
            }

            RpcResponse rpcResponse = null;

            //parse protobuf
            try {
                Head.CRpcHead pbHead = Head.CRpcHead.parseFrom(headBytes);
                String serviceMethodName = pbHead.getMethodName().toStringUtf8();
                //String[] splits = serviceMethodName.split(":");
                //String serviceName = splits[0];
                //String methodName = splits[1];

                rpcResponse = new RpcResponse();
                rpcResponse.setSeq(pbHead.getSequence());
                rpcResponse.setErrno(pbHead.getErr());
                if (pbHead.getErrMsg() != null && !pbHead.getErrMsg().isEmpty()) {
                    rpcResponse.setError(new Exception(pbHead.getErrMsg().toStringUtf8()));
                } else {
                rpcResponse.setError(null);
                }

                if (pbHead.getErr() == 0) {
                    rpcResponse.setResultObj(bodyBytes);
                } else {
                    rpcResponse.setResultObj(null);
                }

             } catch (com.google.protobuf.InvalidProtocolBufferException ex) {
                rpcResponse = null;
                log.error("RPC Response parse failed.");
                throw new IllegalArgumentException("Response decode error: protobuf parse failed.");
            }

            if (rpcResponse != null) {
                if (messages == null) {
                    messages = new ArrayList<Object>();
                }
                messages.add(rpcResponse);
                lastReadIndex = cb.readerIndex();
            } else {
                setAttachment(channelHandlerContext, channel, cb, lastReadIndex);
                break;
            }
        }

        return messages;
    }

    private RpcResponse getCustomPackage(ChannelHandlerContext channelHandlerContext, Channel channel, ChannelBuffer cb, ChannelHandlerContext encodeCtx) {
        byte[] tmp_buff = new byte[cb.readableBytes()];
        cb.readBytes(tmp_buff);
        
        CustomPackageLengthChecker  lengthChecker = (CustomPackageCodec) NettyCodecUtils.getAttachment(encodeCtx, Constants.ATTACHMENT_CUSTOM_PROTO_CODEC);
        int checkRet = lengthChecker.checkPackageLength(tmp_buff);
        if (checkRet < 0) {
            return null;
        } else if (checkRet == 0 || checkRet > tmp_buff.length) {
            cb.resetReaderIndex();
            setAttachment(channelHandlerContext, channel, cb, 0);
            return null;
        } else if (checkRet <= tmp_buff.length) {
            if (checkRet < tmp_buff.length) {
                tmp_buff = null;
                tmp_buff = new byte[checkRet];
                cb.resetReaderIndex();
                cb.readBytes(tmp_buff);
            }

            long sequence = 0;
            Long sequenceObj = (Long)NettyCodecUtils.getAttachment(encodeCtx, Constants.ATTACHMENT_CUSTOM_PROTO_SEQUENCE);
            if (sequenceObj == null) {
                CustomPackageCodec packageCodec = (CustomPackageCodec)lengthChecker;
                sequence = packageCodec.decodeSequence(tmp_buff);
            } else {
                sequence = sequenceObj.longValue();
            }
            RpcResponse rpcResponse = new RpcResponse();
            rpcResponse.setSeq(sequence);
            rpcResponse.setResultObj(tmp_buff);
            return rpcResponse;
        }
        return null;
    }

    private void setAttachment(ChannelHandlerContext ctx, Channel channel, ChannelBuffer cb, int lastReadIndex) {
        cb.readerIndex(lastReadIndex);
        if (!(cb instanceof DynamicChannelBuffer) || cb.writerIndex() > 102400) {
            ChannelBuffer db = ChannelBuffers.dynamicBuffer(cb.readableBytes() * 2, channel.getConfig().getBufferFactory());
            db.writeBytes(cb);
            cb = db;
        }

        NettyCodecUtils.setAttachment(ctx, Constants.ATTACHMENT_BYTEBUFFER, cb);
    }
}
