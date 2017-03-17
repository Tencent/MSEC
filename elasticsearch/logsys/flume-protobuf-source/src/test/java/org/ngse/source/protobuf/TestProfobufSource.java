
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

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.text.SimpleDateFormat;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

import org.apache.flume.Channel;
import org.apache.flume.ChannelSelector;
import org.apache.flume.Context;
import org.apache.flume.Sink;
import org.apache.flume.SinkRunner;
import org.apache.flume.channel.ChannelProcessor;
import org.apache.flume.channel.MemoryChannel;
import org.apache.flume.channel.ReplicatingChannelSelector;
import org.apache.flume.conf.Configurables;
import org.apache.flume.sink.DefaultSinkProcessor;
import org.apache.flume.sink.LoggerSink;
import org.ngse.source.protobuf.ProtobufSource;
import org.junit.Assert;
import org.junit.Assume;
import org.junit.BeforeClass;
import org.junit.Test;

public class TestProfobufSource extends Assert {

  @BeforeClass
  public static void setUp() {
  }
  
  @Test
  public void testBasic() throws Exception {
    Context context = new Context();
    context.put("bind", "localhost");
    context.put("port", "45673");
    context.put("threads", "1");

    ProtobufSource source = new ProtobufSource();
    source.configure(context);

    Map<String, String> channelContext = new HashMap();
    channelContext.put("capacity", "1000000");
    channelContext.put("keep-alive", "0"); // for faster tests
    Channel channel = new MemoryChannel();
    Configurables.configure(channel, new Context(channelContext));

    Sink sink = new LoggerSink();
    sink.setChannel(channel);
    sink.start();
    DefaultSinkProcessor proc = new DefaultSinkProcessor();
    proc.setSinks(Collections.singletonList(sink));
    SinkRunner sinkRunner = new SinkRunner(proc);
    sinkRunner.start();

    ChannelSelector rcs = new ReplicatingChannelSelector();
    rcs.setChannels(Collections.singletonList(channel));
    ChannelProcessor chp = new ChannelProcessor(rcs);
    source.setChannelProcessor(chp);
    source.start();

    Thread.sleep(5000);
    source.stop();
    sinkRunner.stop();
    sink.stop();
  }
}
