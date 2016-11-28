
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

import api.monitor.msec.org.AccessMonitor;
import com.google.protobuf.MessageLite;
import org.apache.log4j.Logger;
import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.buffer.ChannelBufferOutputStream;
import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.oneone.OneToOneEncoder;
import org.msec.rpc.RpcRequest;
import org.msec.rpc.ServiceFactory;
import srpc.Head;
import com.google.protobuf.ByteString;

import java.io.IOException;


public class RequestEncoder extends OneToOneEncoder {
    private static Logger log = Logger.getLogger(RequestEncoder.class.getName());



    protected int encodeHead(RpcRequest rpcRequest, ChannelBufferOutputStream stream) throws IOException {
        //TODO: set coloring, color_id & caller..
        //System.out.println("Start encode request head: " + System.currentTimeMillis());

        String serviceMethodName = rpcRequest.getServiceName() + "." + rpcRequest.getMethodName();
        long colorId = NettyCodecUtils.generateColorId(serviceMethodName);
        Head.CRpcHead.Builder builder = Head.CRpcHead.newBuilder();
        builder.clear();
        builder.setSequence(rpcRequest.getSeq());
        builder.setColoring(0);
        builder.setColorId(colorId);
        if (rpcRequest.getFlowid() == 0) {
            builder.setFlowId(rpcRequest.getSeq() ^ colorId);
        } else {
            builder.setFlowId(rpcRequest.getFlowid());
        }

        builder.setErr(0);
        builder.setResult(0);
        builder.setErrMsg(ByteString.EMPTY);
        builder.setCaller(ByteString.copyFromUtf8(ServiceFactory.getModuleName()));
        builder.setMethodName(ByteString.copyFrom((serviceMethodName).getBytes()));
        builder.addCallerStack(ByteString.EMPTY);

        //System.out.println("Start encode request head3: " + System.currentTimeMillis());
        Head.CRpcHead rpchead = builder.build();
        stream.write(rpchead.toByteArray());
        //System.out.println("Start encode request head4: " + System.currentTimeMillis());
		AccessMonitor.add("frm.rpc call to " + rpcRequest.getServiceName() + "/" + rpcRequest.getMethodName());
        return rpchead.getSerializedSize();
    }

    protected int encodeBody(RpcRequest rpcRequest, ChannelBufferOutputStream stream) throws IOException {
        MessageLite message = (MessageLite)rpcRequest.getParameter();
        stream.write(message.toByteArray());
        return message.getSerializedSize();
    }

    protected ChannelBuffer encode(ChannelHandlerContext channelHandlerContext, RpcRequest rpcRequest, ChannelBufferOutputStream stream) throws IOException {
        stream.writeByte('(');
        stream.writeLong(0);

        int headLen = encodeHead(rpcRequest, stream);
        int bodyLen = encodeBody(rpcRequest, stream);

        stream.writeByte(')');

        ChannelBuffer buffer = stream.buffer();
        buffer.setInt(1, headLen);
        buffer.setInt(5, bodyLen);
        return buffer;
    }

    private static final int estimatedLength = 1024;
    @Override
    protected Object encode(ChannelHandlerContext ctx, Channel channel, Object msg) throws Exception {
        if (msg instanceof RpcRequest) {
            RpcRequest message = (RpcRequest) msg;
            try {
                ChannelBufferOutputStream bout = new ChannelBufferOutputStream(ChannelBuffers.dynamicBuffer(estimatedLength, ctx.getChannel()
                        .getConfig().getBufferFactory()));

                //Set proto flag = 0
                NettyCodecUtils.setAttachment(ctx, Constants.ATTACHMENT_CUSTOM_PROTO_FLAG, new Integer(0));
                ChannelBuffer ret = encode(ctx, message, bout);
                return ret;
            } catch (Exception ex) {
                log.error(ex.getMessage(), ex);
                throw ex;
            }
        }

        if (msg instanceof Object[]) {
            Object[] objects = (Object[])msg;
            //Set proto flag=1
            NettyCodecUtils.setAttachment(ctx, Constants.ATTACHMENT_CUSTOM_PROTO_FLAG, new Integer(1));
            NettyCodecUtils.setAttachment(ctx, Constants.ATTACHMENT_CUSTOM_PROTO_CODEC, objects[1]);
            NettyCodecUtils.setAttachment(ctx, Constants.ATTACHMENT_CUSTOM_PROTO_SEQUENCE, objects[2]);

            if (objects[0] instanceof  byte[]) {
                byte[] msgData = (byte[])objects[0];
                int tmpLength = estimatedLength;
                if (msgData.length > tmpLength) {
                    tmpLength = msgData.length;
                }

                ChannelBufferOutputStream bout = new ChannelBufferOutputStream(ChannelBuffers.dynamicBuffer(tmpLength,
                        ctx.getChannel().getConfig().getBufferFactory()));
                bout.write(msgData);
                return bout.buffer();
            } else {
                return objects[0];
            }
        }
        return null;
    }
}
