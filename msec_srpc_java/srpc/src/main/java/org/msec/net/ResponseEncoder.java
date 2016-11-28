
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

import com.google.protobuf.ByteString;
import com.google.protobuf.Message;
import com.google.protobuf.MessageLite;
import com.googlecode.protobuf.format.JsonFormat;
import org.apache.log4j.Logger;
import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.buffer.ChannelBufferOutputStream;
import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.oneone.OneToOneEncoder;
import org.msec.rpc.HttpResponse;
import org.msec.rpc.RpcRequest;
import org.msec.rpc.RpcResponse;
import org.msec.rpc.ServiceFactory;
import srpc.Head;

import java.io.IOException;


public class ResponseEncoder extends OneToOneEncoder {
    private static Logger log = Logger.getLogger(ResponseEncoder.class.getName());

    protected int encodeProtobufHead(RpcResponse rpcResponse, ChannelBufferOutputStream stream) throws IOException {
        //TODO: set coloring, color_id & caller..
        Head.CRpcHead.Builder builder = Head.CRpcHead.newBuilder();
        builder.clear();
        builder.setSequence(rpcResponse.getSeq());
        builder.setColoring(0);
        builder.setColorId(0);
        builder.setFlowId(0);
        builder.setErr(rpcResponse.getErrno());
        builder.setResult(0);

        if (rpcResponse.getErrno() != 0) {
            if (rpcResponse.getError() != null && rpcResponse.getError().getMessage() != null) {
                builder.setErrMsg(ByteString.copyFromUtf8(rpcResponse.getError().getMessage()));
            } else {
                builder.setErrMsg(ByteString.copyFromUtf8("Unknown exception!"));
            }
        }
        else {
            builder.setErrMsg(ByteString.EMPTY);
        }
        builder.setCaller(ByteString.copyFromUtf8(ServiceFactory.getModuleName()));
        builder.setMethodName(ByteString.EMPTY);
        builder.addCallerStack(ByteString.EMPTY);

        Head.CRpcHead rpchead = builder.build();
        stream.write(rpchead.toByteArray());
        return rpchead.getSerializedSize();
    }

    protected int encodeProtobufBody(RpcResponse rpcResponse, ChannelBufferOutputStream stream) throws IOException {
        if (rpcResponse.getResultObj() == null) {
            return 0;
        }

        MessageLite message = (MessageLite)rpcResponse.getResultObj();
        stream.write(message.toByteArray());
        return message.getSerializedSize();
    }

    protected ChannelBuffer serializeProtobufPakcage(ChannelHandlerContext channelHandlerContext, RpcResponse response, ChannelBufferOutputStream stream) throws IOException {
        stream.writeByte('(');
        stream.writeLong(0);

        int headLen = encodeProtobufHead(response, stream);
        int bodyLen = encodeProtobufBody(response, stream);

        stream.writeByte(')');

        ChannelBuffer buffer = stream.buffer();
        buffer.setInt(1, headLen);
        buffer.setInt(5, bodyLen);
        return buffer;
    }

    protected ChannelBuffer serializeHTTPPakcage(ChannelHandlerContext channelHandlerContext, RpcResponse response, ChannelBufferOutputStream stream) throws IOException {

        HttpResponse  httpResponse = new HttpResponse();
        String body = "";
        httpResponse.setStatusCode("200 OK");

        if (response.getResultObj() != null &&  !(response.getResultObj() instanceof Message)) {
            //If result object is not protobuf message, just call toString()
            httpResponse.setContentType("text/html; charset=utf-8");
            body = response.getResultObj().toString();
        } else {
            //If result object is protobuf message, transfer it into json
            httpResponse.setContentType("application/json");

            body = "{\"ret\":" + response.getErrno();

            body += ", \"errmsg\": \"";
            if (response.getError() != null)
                body += response.getError().getMessage();
            body += "\"";

            body += ", \"resultObj\":";
            if (response.getResultObj() != null && response.getResultObj() instanceof Message) {
                body += JsonFormat.printToString((Message) response.getResultObj());
            } else {
                body += "{}";
            }

            body += "}";
        }

        httpResponse.setBody(body);
        stream.writeBytes(httpResponse.write());
        return stream.buffer();
    }

    private static final int estimatedLength = 10240;
    @Override
    protected Object encode(ChannelHandlerContext ctx, Channel channel, Object msg) throws Exception {
        if (msg instanceof RpcResponse) {
            RpcResponse message = (RpcResponse) msg;
            try {
                ChannelBufferOutputStream bout = new ChannelBufferOutputStream(ChannelBuffers.dynamicBuffer(estimatedLength, ctx.getChannel()
                        .getConfig().getBufferFactory()));
                ChannelBuffer buffer;
                if (message.getSerializeMode() == RpcRequest.SerializeMode.SERIALIZE_MODE_PROTOBUF) {
                    buffer = serializeProtobufPakcage(ctx, message, bout);
                } else {
                    buffer = serializeHTTPPakcage(ctx, message, bout);
                }

                return buffer;
            } catch (Exception ex) {
                log.error(ex.getMessage(), ex);
                throw ex;
            }
        }
        return null;
    }
}
