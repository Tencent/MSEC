
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

import org.jboss.netty.channel.ChannelHandlerContext;

import java.net.InetSocketAddress;
import java.util.HashMap;
import java.util.Map;


public class RpcContext {
    public RpcContext(RpcRequest request, ChannelHandlerContext channelContext) {
        this.request = request;
        this.channelContext = channelContext;

        clientAddr = (InetSocketAddress)channelContext.getChannel().getRemoteAddress();
        localAddr = (InetSocketAddress)channelContext.getChannel().getLocalAddress();
        InitLogOptions();
    }

    private RpcRequest request;
    private RpcResponse response;
    private InetSocketAddress clientAddr;
    private InetSocketAddress localAddr;
    private ChannelHandlerContext  channelContext;

    private Map<String, String> logOptions = new HashMap<String, String>();

    private  void InitLogOptions() {
        logOptions.put("ReqID", Long.toString(request.getFlowid()));
        logOptions.put("ClientIP", clientAddr.getAddress().getHostAddress());
        logOptions.put("ServerIP", localAddr.getAddress().getHostAddress());
        logOptions.put("RPCName", request.getServiceName() + "." + request.getMethodName());
        logOptions.put("Caller", request.getFromModule());
        logOptions.put("ServiceName", ServiceFactory.getModuleName());
    }

    public RpcRequest getRequest() {
        return request;
    }

    public ChannelHandlerContext getChannelContext() {
        return channelContext;
    }

    public RpcResponse getResponse() {
        return response;
    }

    public void setResponse(RpcResponse response) {
        this.response = response;
    }

    public InetSocketAddress getClientAddr() {
        return clientAddr;
    }

    public void setClientAddr(InetSocketAddress clientAddr) {
        this.clientAddr = clientAddr;
    }

    public InetSocketAddress getLocalAddr() {
        return localAddr;
    }

    public void setLocalAddr(InetSocketAddress localAddr) {
        this.localAddr = localAddr;
    }

    public Map<String, String> getLogOptions() {
        return logOptions;
    }
}
