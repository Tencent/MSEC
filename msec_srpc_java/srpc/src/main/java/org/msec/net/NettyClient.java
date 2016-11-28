
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
import org.jboss.netty.bootstrap.ClientBootstrap;
import org.jboss.netty.buffer.ChannelBufferOutputStream;
import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.channel.*;
import org.jboss.netty.channel.socket.nio.NioClientSocketChannelFactory;
import org.msec.rpc.*;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class NettyClient {
    private static Logger log = Logger.getLogger(NettyClient.class.getName());

    private String host = "127.0.0.1";
    private int port = 7963;

    private ClientBootstrap bootstrap;
    private boolean connected = false;
    private Channel connectChannel;
    public static final int connectTimeoutInMS = 1000;
    public static int requestTimeoutInMS = 500;

    public static Map<Long, Callback> sessions = new ConcurrentHashMap<Long, Callback>();

    private static ExecutorService bossExecutor = Executors.newCachedThreadPool();

    private static ExecutorService workExecutor = Executors.newCachedThreadPool();

    private static ChannelFactory channelFactory = new NioClientSocketChannelFactory(bossExecutor,
            workExecutor, 1, Runtime.getRuntime().availableProcessors());

    public NettyClient(String host, int port){
        this.host = host;
        this.port = port;
        this.bootstrap = new ClientBootstrap(channelFactory);;

        this.bootstrap.setPipelineFactory(new NettyClientPipelineFactory(this));
        this.bootstrap.setOption("tcpNoDelay", true);
        this.bootstrap.setOption("keepAlive", true);
        this.bootstrap.setOption("reuseAddress", true);
        this.bootstrap.setOption("connectTimeoutMillis", connectTimeoutInMS);
    }

    public synchronized boolean connect() {
        //System.out.println("Connect begin: " + System.currentTimeMillis());
        if (connected) return true;
        ChannelFuture future = null;
        //log.info("Connect begin: " + System.currentTimeMillis());

        try {
            future = bootstrap.connect(new InetSocketAddress(host, port));
            //log.info("Connect called: " + System.currentTimeMillis());
            if (future.awaitUninterruptibly(connectTimeoutInMS, TimeUnit.MILLISECONDS)) {
                if (future.isSuccess()) {
                    Channel newChannel = future.getChannel();
                    try {
                        //Close old connections
                        Channel oldChannel = this.connectChannel;
                        if (oldChannel != null) {
                            //log.info("close old channel " + oldChannel);
                            try {
                                oldChannel.close();
                            } catch (Throwable t) {
                            }
                        }
                    } finally {
                        this.connectChannel = newChannel;
                    }
                    //log.debug("client is connected to " + this.host + ":" + this.port);
                    this.connected = true;
                } else {
                    //log.debug("client is not connected to " + this.host + ":" + this.port);
                }
            } else {
                log.error("timeout while connecting to " + this.host + ":" + this.port);
            }
        } catch (Throwable e) {
            log.error("failed to connect to " + host + ":" + port, e);
        }
        //log.info("Connect end: " + System.currentTimeMillis());
        return connected;
    }

    public ChannelFuture sendRequest(Object request) {
        ChannelFuture future = null;
        if (connectChannel == null) {
            System.out.println("sending request but no connection!");
            return null;
        }
        try {
            future = connectChannel.write(request);
            //future.addListener(new MsgWriteListener(request));
        } catch (Exception ex) {
            System.out.println("write channel failed.");
            connected = false;
            return null;
        }
        return future;
    }

    public RpcResponse sendRequestAndWaitResponse(RpcRequest request, int timeoutMillis)
    {
        if (sendRequest(request) == null)   return null;
        CallbackFuture callback = new CallbackFuture(request, this);
        sessions.put(request.getSeq(), callback);

        RpcResponse response = null;
        try {
            response = callback.getResponse(timeoutMillis);
        }  catch (Exception ex) {
            //TODO: set exception
            System.out.println("Exception occurs: " + ex);

            response = new RpcResponse();
            response.setErrno(-1);
            response.setError(ex);
        } finally {
            sessions.remove(request.getSeq());
        }

        return response;
    }

    //非RPC协议
    public RpcResponse sendRequestAndWaitResponse(CustomPackageCodec packageCodec, RpcRequest request, int timeoutMillis)
    {
        RpcResponse response = null;
        ChannelBufferOutputStream bout = new ChannelBufferOutputStream(ChannelBuffers.dynamicBuffer(1024));

        long sequence = request.getSeq();
        int ret = 0;
        try {
            ret = packageCodec.encode(sequence, bout);
        } catch (IOException ex) {
            response = new RpcResponse();
            response.setErrno(-1);
            response.setError(ex);
            return response;
        }

        if (ret != 0) {
            response = new RpcResponse();
            response.setErrno(-1);
            response.setError(new Exception("Encode request failed."));
            return response;
        }

        Object[]  objects = new Object[3];
        objects[0] = bout.buffer();
        objects[1] = packageCodec;
        objects[2] = null;
        if (sendRequest(objects) == null)   return null;
        CallbackFuture callback = new CallbackFuture(request, this);
        sessions.put(request.getSeq(), callback);

        try {
            response = callback.getResponse(timeoutMillis);
        }  catch (Exception ex) {
            //TODO: set exception
            System.out.println("Exception occurs: " + ex);

            response = new RpcResponse();
            response.setErrno(-1);
            response.setError(ex);
        } finally {
            sessions.remove(request.getSeq());
        }

        return response;
    }

    //非RPC协议
    public RpcResponse sendRequestAndWaitResponse(CustomPackageLengthChecker lengthChecker, byte[] requestData, RpcRequest request, int timeoutMillis)
    {
        RpcResponse response = null;
        ChannelBufferOutputStream bout = new ChannelBufferOutputStream(ChannelBuffers.dynamicBuffer(1024));

        Object[]  objects = new Object[3];
        objects[0] = requestData;
        objects[1] = lengthChecker;
        objects[2] = new Long(request.getSeq());
        if (sendRequest(objects) == null)   return null;
        CallbackFuture callback = new CallbackFuture(request, this);
        sessions.put(request.getSeq(), callback);

        try {
            response = callback.getResponse(timeoutMillis);
        }  catch (Exception ex) {
            //TODO: set exception
            System.out.println("Exception occurs: " + ex);

            response = new RpcResponse();
            response.setErrno(-1);
            response.setError(ex);
        } finally {
            sessions.remove(request.getSeq());
        }

        return response;
    }

    public String getHost() {
        return host;
    }

    public int getPort() {
        return port;
    }

    public boolean isConnected() {
        return connected;
    }

    public void setConnected(boolean connected) {
        this.connected = connected;
    }

    public void close() {
        connected = false;
        connectChannel.close();
    }

    public class MsgWriteListener implements ChannelFutureListener {

        private RpcRequest request;

        public MsgWriteListener(RpcRequest request) {
            this.request = request;
        }

        public void operationComplete(ChannelFuture future) throws Exception {
            if (future.isSuccess()) {
                return;
            }
            log.error("operation complete failed.");
        }

    }
}
