
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
import org.msec.rpc.Callback;
import org.msec.rpc.RpcResponse;

import java.util.List;


public class NettyClientHandler extends SimpleChannelUpstreamHandler {
    private static Logger log = Logger.getLogger(NettyServerHandler.class.getName());

    private NettyClient client;
    public NettyClientHandler(NettyClient client) {
        this.client = client;
    }

    @Override
    public void handleUpstream(ChannelHandlerContext ctx, ChannelEvent e) throws Exception {
        super.handleUpstream(ctx, e);
    }

    @Override
    public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception {
    }

    @Override
    public void channelDisconnected(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception {
        client.setConnected(false);
    }

    @Override
    public void channelClosed(ChannelHandlerContext ctx, ChannelStateEvent e) throws Exception {
        client.setConnected(false);
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception {
        List<RpcResponse> messages = (List<RpcResponse>) e.getMessage();
        for (final RpcResponse response : messages) {
            Callback callback = NettyClient.sessions.get(response.getSeq());
            if (callback != null) {
                callback.OnResponse(response);
            }
            else {
                log.error("Look up seq for session failed: " + response.getSeq());
            }
        }
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) throws Exception {
        client.setConnected(false);
        log.error(e.getCause().getMessage(), e.getCause());
    }
}
