
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
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;
import com.google.protobuf.MessageLite;
import com.googlecode.protobuf.format.JsonFormat;
import org.apache.log4j.Logger;
import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.buffer.ChannelBufferIndexFinder;
import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.buffer.DynamicChannelBuffer;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.handler.codec.oneone.OneToOneDecoder;
import org.msec.rpc.HttpFormatException;
import org.msec.rpc.HttpRequestParser;
import org.msec.rpc.RpcRequest;
import org.msec.rpc.ServiceFactory;
import srpc.Head;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;


public class RequestDecoder extends OneToOneDecoder {
    private static Logger log = Logger.getLogger(RequestDecoder.class.getName());

    @Override
    protected Object decode(ChannelHandlerContext channelHandlerContext, Channel channel, Object msg) throws Exception {
        long receiveTime = System.currentTimeMillis();
        if (!(msg instanceof ChannelBuffer)) {
            return msg;
        }

        ChannelBuffer cb = (ChannelBuffer) NettyCodecUtils.getAttachment(channelHandlerContext, Constants.ATTACHMENT_BYTEBUFFER);
        ChannelBuffer cb_ = (ChannelBuffer) msg;
        if (cb == null) {
            cb = cb_;
        } else {
            cb.writeBytes(cb_);
        }

        List<Object> messages = null;
        int lastReadIndex = cb.readerIndex();
        int deserializeMode = -1;
        while (cb.readable()) {
            byte stx = cb.readByte();
            RpcRequest rpcRequest = null;

            if (stx == (byte) '(')  {
                if (cb.readableBytes() < Constants.PKG_LEAST_LENGTH - 1) {
                    setAttachment(channelHandlerContext, channel, cb, lastReadIndex);
                    break;
                }

                //Test whether protocol is protobuf
                //Format:  ( + dwHeadLength + dwBodyLength + strHead + strBody + )
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
                    throw new IllegalArgumentException("Request decode error: invalid package etx " + etx);
                }


                //parse protobuf package
                rpcRequest = deserializeProtobufPackage(headBytes, bodyBytes);
                rpcRequest.setSerializeMode(RpcRequest.SerializeMode.SERIALIZE_MODE_PROTOBUF);
            } else {
                //Test whether protocol is HTTP
                cb.readerIndex(lastReadIndex);
                int totalLength = cb.readableBytes();
                byte[] totalBytes = new byte[totalLength];
                cb.readBytes(totalBytes);

                String total = new String(totalBytes);
                int pos = total.indexOf("\r\n\r\n");

                if (pos < 0) {
                    setAttachment(channelHandlerContext, channel, cb, lastReadIndex);
                    break;
                }

                int contentLength = getHTTPContentLength(total.substring(0, pos + 4));
                if (totalLength < pos + 4 + contentLength) {
                    setAttachment(channelHandlerContext, channel, cb, lastReadIndex);
                    break;
                }
                cb.readerIndex(pos + 4 + contentLength);

                //parse HTTP package
                rpcRequest = deserializeHTTPPackage(total.substring(0, pos + 4 + contentLength));
                rpcRequest.setSerializeMode(RpcRequest.SerializeMode.SERIALIZE_MODE_HTTP);
            }

            if (rpcRequest != null) {
                if (messages == null) {
                    messages = new ArrayList<Object>();
                }
                messages.add(rpcRequest);
                lastReadIndex = cb.readerIndex();
            } else {
                setAttachment(channelHandlerContext, channel, cb, lastReadIndex);
                break;
            }
        }

        return messages;
    }

    private RpcRequest deserializeHTTPPackage(String  request) {
        RpcRequest rpcRequest = new RpcRequest();
        HttpRequestParser httpParser = new HttpRequestParser();
        try {
            httpParser.parseRequest(request);
        } catch (IOException ex) {
            log.error("HTTP Request parse failed." + ex.getMessage());
            rpcRequest.setException(new IllegalArgumentException("Request decode error: HTTP Request parse failed."  + ex.getMessage()));
            return rpcRequest;
        } catch (HttpFormatException ex) {
            log.error("HTTP Request parse failed." + ex.getMessage());
            rpcRequest.setException(new IllegalArgumentException("Request decode error: HTTP Request parse failed."  + ex.getMessage()));
            return rpcRequest;
        }

        rpcRequest.setHttpCgiName(httpParser.getCgiPath());
        if (httpParser.getCgiPath().compareToIgnoreCase("/list") == 0) {
            return rpcRequest;
        }

        if (httpParser.getCgiPath().compareToIgnoreCase("/invoke") != 0) {
            log.error("HTTP cgi not found: " + httpParser.getCgiPath());
            rpcRequest.setException(new IllegalArgumentException("HTTP cgi not found: " + httpParser.getURI()));
            return rpcRequest;
        }
        String serviceMethodName = httpParser.getQueryString("methodName");
        int pos = serviceMethodName.lastIndexOf('.');
        if (pos == -1 || pos == 0 || pos == serviceMethodName.length() - 1) {
            log.error("Invalid serviceMethodName (" + serviceMethodName + "). Must be in format like *.*");
            rpcRequest.setException(new IllegalArgumentException("Invalid serviceMethodName (" + serviceMethodName + "). Must be in format like *.*"));
            return rpcRequest;
        }
        String serviceName = serviceMethodName.substring(0, pos);
        String methodName = serviceMethodName.substring(pos + 1);
        String seq = httpParser.getQueryString("seq");
        String param = httpParser.getQueryString("param");
        if (param == null || param.isEmpty()) {
            param = httpParser.getMessageBody();
        }
        
        try {
            param = java.net.URLDecoder.decode(param, "UTF-8");
        } catch (UnsupportedEncodingException e) {
            param = "";
        }

        rpcRequest.setServiceName(serviceName);
        rpcRequest.setMethodName(methodName);
        if (seq == null || seq.isEmpty()) {
            rpcRequest.setSeq(ServiceFactory.generateSequence());
        } else {
            rpcRequest.setSeq(Long.valueOf(seq).longValue());
        }

        log.info("In HTTP package service: " + serviceName + " method: " + methodName + " param: " + param);
        rpcRequest.setFromModule("UnknownModule");
        rpcRequest.setFlowid(rpcRequest.getSeq() ^ NettyCodecUtils.generateColorId(serviceName + methodName));
        AccessMonitor.add("frm.rpc request incoming: " + methodName);

        ServiceFactory.ServiceMethodEntry serviceMethodEntry = ServiceFactory.getServiceMethodEntry(serviceName, methodName);
        if (serviceMethodEntry == null) {
            log.error("No service method registered: " + rpcRequest.getServiceName() + "/" + rpcRequest.getMethodName());
            rpcRequest.setException(new IllegalArgumentException("No service method registered: " + rpcRequest.getServiceName()
                    + "/" + rpcRequest.getMethodName()));
            return rpcRequest;
        }

        Message.Builder  builder = serviceMethodEntry.getParamTypeBuilder();
        try {
            builder.clear();
            JsonFormat.merge(param, builder);
        } catch (JsonFormat.ParseException ex) {
            log.error("Json parse failed." + ex.getMessage());
            rpcRequest.setException(new IllegalArgumentException("Request decode error: Json parse failed."  + ex.getMessage()));
            return rpcRequest;
        }

        if ( !builder.isInitialized() ) {
            log.error("Json to Protobuf failed: missing required fields");
            rpcRequest.setException(new IllegalArgumentException("Json to Protobuf failed: missing required fields: "  +
                    builder.getInitializationErrorString()));
            return rpcRequest;
        }
        rpcRequest.setParameter(builder.build());
        log.info("RPC Request received. ServiceMethodName: " + rpcRequest.getServiceName() + "/" + rpcRequest.getMethodName() +
                "\tSeq: " + rpcRequest.getSeq() + "\tParameter: " + rpcRequest.getParameter());
        return rpcRequest;
    }

    private RpcRequest deserializeProtobufPackage(byte[]  headBytes, byte[]  bodyBytes)
    {
        Head.CRpcHead pbHead = null;
        RpcRequest rpcRequest = new RpcRequest();

        try {
            pbHead = Head.CRpcHead.parseFrom(headBytes);
            rpcRequest.setSeq(pbHead.getSequence());
        } catch (InvalidProtocolBufferException e) {
            log.error("Parse protobuf head failed.");
            rpcRequest.setException(new IllegalArgumentException("Parse protobuf head failed."));
            return rpcRequest;
        }

        String serviceMethodName = pbHead.getMethodName().toStringUtf8();
        int pos = serviceMethodName.lastIndexOf('.');
        if (pos == -1 || pos == 0 || pos == serviceMethodName.length() - 1) {
            log.error("Invalid serviceMethodName (" + serviceMethodName + "). Must be in format like *.*");
            rpcRequest.setException(new IllegalArgumentException("Invalid serviceMethodName (" + serviceMethodName + "). Must be in format like *.*"));
            return rpcRequest;
        }
        String serviceName = serviceMethodName.substring(0, pos);
        String methodName = serviceMethodName.substring(pos + 1);

        rpcRequest.setServiceName(serviceName);
        rpcRequest.setMethodName(methodName);
        rpcRequest.setFlowid(pbHead.getFlowId());
        if (pbHead.getCaller() != null && !pbHead.getCaller().isEmpty()) {
            rpcRequest.setFromModule(pbHead.getCaller().toStringUtf8());
        } else {
            rpcRequest.setFromModule("UnknownModule");
        }

        //If flowid == 0, we must generate one
        if (rpcRequest.getFlowid() == 0) {
            rpcRequest.setFlowid(rpcRequest.getSeq() ^ NettyCodecUtils.generateColorId(serviceMethodName));
        }

        AccessMonitor.add("frm.rpc request incoming: " + methodName);

        ServiceFactory.ServiceMethodEntry serviceMethodEntry = ServiceFactory.getServiceMethodEntry(serviceName, methodName);
        if (serviceMethodEntry == null) {
            log.error("No service method registered: " + rpcRequest.getServiceName() + "/" + rpcRequest.getMethodName());
            rpcRequest.setException(new IllegalArgumentException("No service method registered: " + rpcRequest.getServiceName()
                    + "/" + rpcRequest.getMethodName()));
            return rpcRequest;
        }

        MessageLite param = null;
        try {
            param = (MessageLite) serviceMethodEntry.getParamTypeParser().parseFrom(bodyBytes);
        } catch (InvalidProtocolBufferException ex) {
            log.error("Parse protobuf body failed.");
            rpcRequest.setException(new IllegalArgumentException("RParse protobuf body failed."  + ex.getMessage()));
            return rpcRequest;
        }
        rpcRequest.setParameter(param);
        log.info("RPC Request received. ServiceMethodName: " + rpcRequest.getServiceName() + "/" + rpcRequest.getMethodName() +
                "\tSeq: " + rpcRequest.getSeq() + "\tParameter: " + rpcRequest.getParameter());
        return rpcRequest;
    }

    private int getHTTPContentLength(String req) {
        int ret = 0;
        if ( !req.startsWith("POST") )
            return ret;

        String keyStr = "Content-Length:";
        int spos = req.indexOf(keyStr);
        if (spos < 0)
            return ret;

        spos += keyStr.length();
        while (req.charAt(spos) == ' ')  spos++;

        if ( !Character.isDigit(req.charAt(spos)) )
            return ret;

        int epos = spos;
        while (Character.isDigit(req.charAt(epos)))  epos++;
        ret = Integer.parseInt(req.substring(spos, epos));
        return ret;
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
