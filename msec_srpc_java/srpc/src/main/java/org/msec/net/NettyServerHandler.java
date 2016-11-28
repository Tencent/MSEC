
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
import org.jboss.netty.channel.*;
import org.msec.rpc.RequestProcessor;
import org.msec.rpc.RpcRequest;

import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;


public class NettyServerHandler extends SimpleChannelUpstreamHandler {
    private static Logger log = Logger.getLogger(NettyServerHandler.class.getName());

    private NettyServer server;
    private static ExecutorService threadPool = Executors.newCachedThreadPool();
    public NettyServerHandler(NettyServer server) {
        this.server = server;
    }
    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent message) throws Exception {
        List<RpcRequest> messages = (List<RpcRequest>) (message.getMessage());
        for (RpcRequest request : messages) {
            threadPool.submit(new RequestProcessor(ctx, request));
        }
    }

    @Override
    public void channelOpen(ChannelHandlerContext ctx, ChannelStateEvent e) {
        this.server.getChannelGroup().add(e.getChannel());
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) {
        log.error(e.getCause().getMessage(), e.getCause());
    }
}
