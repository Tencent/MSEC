
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

import java.net.InetSocketAddress;
import java.util.concurrent.Executors;

import org.apache.log4j.Logger;
import org.jboss.netty.bootstrap.ServerBootstrap;
import org.jboss.netty.channel.Channel;
import org.jboss.netty.channel.ChannelFactory;
import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.group.ChannelGroup;
import org.jboss.netty.channel.group.DefaultChannelGroup;
import org.jboss.netty.channel.socket.nio.NioServerSocketChannelFactory;



public class NettyServer  {

    private static Logger log = Logger.getLogger(NettyServer.class.getName());

    private String ip = null;
    private int port = 20000;

    private ServerBootstrap bootstrap;
    private ChannelGroup channelGroup = new DefaultChannelGroup();
    private Channel channel;

    private static ChannelFactory channelFactory = new NioServerSocketChannelFactory(Executors.newCachedThreadPool(),
            Executors.newCachedThreadPool());

    private boolean started = false;

    public NettyServer(int port /*ServiceRepository sr, RemoteInvocationHandler invocationHandler*/){
        this(null, port);
    }

    public NettyServer(String ip, int port /*ServiceRepository sr, RemoteInvocationHandler invocationHandler*/){
        this.ip = ip;
        this.port = port;
        this.bootstrap = new ServerBootstrap(channelFactory);

        ChannelPipeline pipeline = bootstrap.getPipeline();
        pipeline.addLast("decode", new RequestDecoder());
        pipeline.addLast("encode", new ResponseEncoder());
        pipeline.addLast("handler", new NettyServerHandler(NettyServer.this));

        this.bootstrap.setOption("backlog", 8102);
        this.bootstrap.setOption("child.tcpNoDelay", true);
        this.bootstrap.setOption("child.keepAlive", true);
        this.bootstrap.setOption("child.reuseAddress", true);
        this.bootstrap.setOption("child.connectTimeoutMillis", 1000);
    }

    public void start() {
        if(!started){
            InetSocketAddress address = null;
            if(this.ip == null){
                address = new InetSocketAddress(this.port);
                log.info("Netty-Server started listening at 0.0.0.0:" + this.port);
            }else{
                address = new InetSocketAddress(this.ip,this.port);
                log.info("Netty-Server started listening at " + this.ip + ":" + this.port);
            }
            channel = this.bootstrap.bind(address);
            this.started = true;
            //Runtime.getRuntime().addShutdownHook(new Thread(new HookRunnable(this)));
        }

    }

    public void stop() {
        if (this.started) {
            if (channelGroup != null) {
                channelGroup.unbind().awaitUninterruptibly();
                channelGroup.close().awaitUninterruptibly();
            }
            if (channel != null) {
                channel.unbind();
            }
            if (bootstrap != null) {
                bootstrap.releaseExternalResources();
            }
            if (channelGroup != null) {
                channelGroup.clear();
            }
            this.started = false;
        }
    }

    public ChannelGroup getChannelGroup() {
        return channelGroup;
    }
}
