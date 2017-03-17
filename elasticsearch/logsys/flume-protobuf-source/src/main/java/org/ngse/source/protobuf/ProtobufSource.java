
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


/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
package org.ngse.source.protobuf;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.channels.Channels;
import java.nio.channels.ClosedByInterruptException;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.Map;
import java.util.HashMap;

import org.apache.flume.ChannelException;
import org.apache.flume.Context;
import org.apache.flume.CounterGroup;
import org.apache.flume.Event;
import org.apache.flume.EventDrivenSource;
import org.apache.flume.FlumeException;
import org.apache.flume.Source;
import org.apache.flume.conf.Configurable;
import org.apache.flume.conf.Configurables;
import org.apache.flume.event.EventBuilder;
import org.apache.flume.source.AbstractSource;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.base.Charsets;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.protobuf.*;

public class ProtobufSource extends AbstractSource implements Configurable,
        EventDrivenSource {

    private static final Logger logger = LoggerFactory
            .getLogger(ProtobufSource.class);

    public static final int MAX_LENGTH = 10240;
    private String hostName;
    private int port;

    private CounterGroup counterGroup;
    private ServerSocketChannel serverSocket;
    private AtomicBoolean acceptThreadShouldStop;
    private Thread acceptThread;
    private ExecutorService handlerService;

    public ProtobufSource() {
        super();

        port = 0;
        counterGroup = new CounterGroup();
        acceptThreadShouldStop = new AtomicBoolean(false);
    }

    @Override
    public void configure(Context context) {
        String hostKey = "bind";
        String portKey = "port";

        Configurables.ensureRequiredNonNull(context, hostKey, portKey);
        hostName = context.getString(hostKey);
        port = context.getInteger(portKey);
    }

    @Override
    public void start() {

        logger.info("Source starting");

        counterGroup.incrementAndGet("open.attempts");

        handlerService = Executors.newCachedThreadPool(new ThreadFactoryBuilder()
                .setNameFormat("protobuf-handler-%d").build());

        try {
            SocketAddress bindPoint = new InetSocketAddress(hostName, port);

            serverSocket = ServerSocketChannel.open();
            serverSocket.socket().setReuseAddress(true);
            serverSocket.socket().bind(bindPoint);

            logger.info("Created serverSocket:{}", serverSocket);
        } catch (IOException e) {
            counterGroup.incrementAndGet("open.errors");
            logger.error("Unable to bind to socket. Exception follows.", e);
            throw new FlumeException(e);
        }

        AcceptHandler acceptRunnable = new AcceptHandler();
        acceptThreadShouldStop.set(false);
        acceptRunnable.counterGroup = counterGroup;
        acceptRunnable.handlerService = handlerService;
        acceptRunnable.shouldStop = acceptThreadShouldStop;
        acceptRunnable.source = this;
        acceptRunnable.serverSocket = serverSocket;

        acceptThread = new Thread(acceptRunnable);

        acceptThread.start();

        logger.debug("Source started");
        super.start();
    }

    @Override
    public void stop() {
        logger.info("Source stopping");

        acceptThreadShouldStop.set(true);

        if (acceptThread != null) {
            logger.debug("Stopping accept handler thread");

            while (acceptThread.isAlive()) {
                try {
                    logger.debug("Waiting for accept handler to finish");
                    acceptThread.interrupt();
                    acceptThread.join(500);
                } catch (InterruptedException e) {
                    logger
                            .debug("Interrupted while waiting for accept handler to finish");
                    Thread.currentThread().interrupt();
                }
            }

            logger.debug("Stopped accept handler thread");
        }

        if (serverSocket != null) {
            try {
                serverSocket.close();
            } catch (IOException e) {
                logger.error("Unable to close socket. Exception follows.", e);
                return;
            }
        }

        if (handlerService != null) {
            handlerService.shutdown();

            logger.debug("Waiting for handler service to stop");

            // wait 500ms for threads to stop
            try {
                handlerService.awaitTermination(500, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                logger
                        .debug("Interrupted while waiting for netcat handler service to stop");
                Thread.currentThread().interrupt();
            }

            if (!handlerService.isShutdown()) {
                handlerService.shutdownNow();
            }

            logger.debug("Handler service stopped");
        }

        logger.debug("Source stopped. Event metrics:{}", counterGroup);
        super.stop();
    }

    private static class AcceptHandler implements Runnable {

        private ServerSocketChannel serverSocket;
        private CounterGroup counterGroup;
        private ExecutorService handlerService;
        private EventDrivenSource source;
        private AtomicBoolean shouldStop;

        public AcceptHandler() {
        }

        @Override
        public void run() {
            logger.debug("Starting accept handler");

            while (!shouldStop.get()) {
                try {
                    SocketChannel socketChannel = serverSocket.accept();

                    ProtobufSocketHandler request = new ProtobufSocketHandler();
                    request.socketChannel = socketChannel;
                    request.counterGroup = counterGroup;
                    request.source = source;

                    handlerService.submit(request);

                    counterGroup.incrementAndGet("accept.succeeded");
                } catch (ClosedByInterruptException e) {
                    // Parent is canceling us.
                } catch (IOException e) {
                    logger.error("Unable to accept connection. Exception follows.", e);
                    counterGroup.incrementAndGet("accept.failed");
                }
            }

            logger.debug("Accept handler exiting");
        }
    }

    private static class ProtobufSocketHandler implements Runnable {

        private Source source;
        private CounterGroup counterGroup;
        private SocketChannel socketChannel;
        byte[] buff;

        public ProtobufSocketHandler() {
            buff = null;
        }

        private int bytesToLong(byte[] bytes) {
            int length = bytes.length;
            int value = 0;
            for (int i = 0; i < length; i++) {
                value += (bytes[i] & 0xff) << (8 * i);
            }
            return value;
        }

        int readExactlyNBytes(java.io.InputStream stream, byte[] bytes, int n, boolean blockOnEOF)
                throws IOException {
            int bytesRead = 0;

            while (bytesRead < n) {
                int numRead = stream.read(bytes, bytesRead, n - bytesRead);
                if (numRead == -1) {
                    break;
                } else {
                    bytesRead += numRead;
                }
            }

            logger.debug("Read " + bytesRead + " bytes from stream");
            return bytesRead;
        }

        @Override
        public void run() {
            logger.debug("Starting connection handler");
            Event event = null;

            try {
                InputStream inputStream = Channels.newInputStream(socketChannel);
                OutputStream outputStream = Channels.newOutputStream(socketChannel);

                while (true) {
                    // Next returns the next event, blocking if none available.
                    byte[] length_bytes = new byte[4];
                    int ret = readExactlyNBytes(inputStream, length_bytes, 4, false);
                    if (ret < 4) {   //End of file
                        break;
                    }

                    int length = bytesToLong(length_bytes);
                    if (length > MAX_LENGTH) {
                        logger.warn("Proto length too long: " + length + " bytes");
                        throw new IOException("Proto length too long: " + length + " bytes");
                    }

                    buff = new byte[length];
                    ret = readExactlyNBytes(inputStream, buff, length, false);
                    if (ret != length) {
                        logger.error("Unexpected end of input.");
                        throw new IOException("Unexpected end of input.");
                    }

                    int eventsProcessed = processEvents(buff, outputStream);
                    if (eventsProcessed > 0) {
                        logger.debug("Events processed succ.");
                    } else {
                        logger.debug("Events processed failed.");
                    }
                }

                socketChannel.close();

                counterGroup.incrementAndGet("sessions.completed");
            } catch (IOException e) {
                counterGroup.incrementAndGet("sessions.broken");
            }

            logger.debug("Connection handler exiting");
        }

        /**
         * <p>Consume some number of events from the buffer into the system.</p>
         *
         * @param buff The buffer containing data to process
         * @param outputStream The stream back to the client
         * @return number of events successfully processed
         * @throws IOException
         */
        private int processEvents(byte[] buff, OutputStream outputStream)
                throws IOException {

            int numProcessed = 0;

            //parse protobuf message

            // build event object
            Event event = null;

            try {
                FlumeProto.FlumeEvent.Builder builder = FlumeProto.FlumeEvent.newBuilder();
                FlumeProto.FlumeEvent pbEvent = FlumeProto.FlumeEvent.parseFrom(buff);
                Map<String, String>  headers = new HashMap<String, String>();
                if (pbEvent != null) {

                    for (int i=0; i<pbEvent.getHeadersCount(); ++i) {
                        FlumeProto.FlumeEventHeader oneHeader = pbEvent.getHeaders(i);
                        headers.put(oneHeader.getKey(), oneHeader.getValue());
                    }
                    event = EventBuilder.withBody(pbEvent.getBody().toByteArray(), headers);
                } else {
                    return 0;
                }
            } catch (InvalidProtocolBufferException ex) {
                return 0;
            }

            // process event
            ChannelException ex = null;
            try {
                source.getChannelProcessor().processEvent(event);
            } catch (ChannelException chEx) {
                ex = chEx;
            }

            if (ex == null) {
                counterGroup.incrementAndGet("events.processed");
                numProcessed++;
            } else {
                counterGroup.incrementAndGet("events.failed");
                logger.warn("Error processing event. Exception follows.", ex);
                //outputStream.write("FAILED: " + ex.getMessage() + "\n");
            }
            outputStream.flush();

            return numProcessed;
        }
    }
}
