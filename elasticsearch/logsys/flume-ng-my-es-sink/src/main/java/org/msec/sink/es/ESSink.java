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

package org.msec.sink.es;

import com.google.common.base.Preconditions;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import org.apache.commons.lang.StringUtils;
import org.apache.flume.*;
import org.apache.flume.conf.Configurable;
import org.apache.flume.sink.AbstractSink;
import org.apache.flume.formatter.output.PathManager;
import org.codehaus.jackson.JsonEncoding;
import org.codehaus.jackson.JsonGenerator;
import org.codehaus.jackson.map.ObjectMapper;
import org.elasticsearch.common.transport.InetSocketTransportAddress;
import org.apache.flume.formatter.output.PathManagerFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.*;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.*;
import java.util.concurrent.*;
import java.lang.String;
import java.text.SimpleDateFormat;

public class ESSink extends AbstractSink implements Configurable {

    private Logger LOG = LoggerFactory.getLogger(ESSink.class);
    public static final int DEFAULT_PORT = 9300;

    //members for elasticsearch config
    private String clusterName = "testCluster";
    private String indexName = "flume";
    private String indexType = "logs";
    private int batchSize;
    private String  indexRollingTime = "1day";

    private Map<String, Integer>  lastIndexRollingMinute = new HashMap<String, Integer>();
    private Map<String, String>   lastIndexRollingName = new HashMap<String, String>();

    private String currentIndexName = "";
    private String[] serverAddressStrings = null;

    private InetSocketTransportAddress[] serverAddresses;

    private int bulkNum;
    private int totalCount = 0;
    private long timeStart = 0;
    private int maxContentLength;

    //private OutputStream outputStream;
    private ScheduledExecutorService rollService;
    private PathManager pathController;
    private volatile boolean bulkTimeout;

    private static ExecutorService threadPool = Executors.newCachedThreadPool();
    private static BlockingQueue< ESClientThread.ESThreadRequest> workingQueue;
    ESClientThread.ESThreadRequest esThreadRequest = new ESClientThread.ESThreadRequest();

    public ESSink() {
        LOG.info("ESSink constructed...");
        bulkTimeout = false;
        bulkNum = 1;
        timeStart = System.currentTimeMillis();
    }

    @Override
    public void configure(Context context) {
        pathController = PathManagerFactory.getInstance("DEFAULT", context);

        if (StringUtils.isNotBlank(context.getString("hosts"))) {
            serverAddressStrings = StringUtils.deleteWhitespace(context.getString("hosts")).split(",");
        }
        Preconditions.checkState(serverAddressStrings != null
                && serverAddressStrings.length > 0, "Missing Param:" + "hosts");

        if (StringUtils.isNotBlank(context.getString("indexName"))) {
            this.indexName = context.getString("indexName");
        }

        if (StringUtils.isNotBlank(context.getString("indexType"))) {
            this.indexType = context.getString("indexType");
        }

        if (StringUtils.isNotBlank(context.getString("clusterName"))) {
            this.clusterName = context.getString("clusterName");
        }

        if (StringUtils.isNotBlank(context.getString("indexRollingTime"))) {
            this.indexRollingTime = context.getString("indexRollingTime");
        }

        bulkNum = context.getInteger("bulkNum", 1);

        batchSize = context.getInteger("batchSize", 100);
        Preconditions.checkNotNull(batchSize > 0, "batchSize must be a positive number!!");

        maxContentLength = context.getInteger("maxContentLength", 1000);
        Preconditions.checkNotNull(maxContentLength > 0, "maxContentLength must be a positive number!!");
    }

    @Override
    public void start() {
        super.start();

        initESThreadPool();

        rollService = Executors.newScheduledThreadPool(
                1,
                new ThreadFactoryBuilder().setNameFormat("ESSink-Bulk-Timer" + Thread.currentThread().getId() + "-%d").build());

        rollService.scheduleAtFixedRate(new Runnable() {
            @Override
            public void run() {
                //LOG.debug("Marking time to bulk");
                bulkTimeout = true;
            }
        }, 3, 3, TimeUnit.SECONDS);

        LOG.info("ESSink {} started.", getName());
    }

    @Override
    public void stop() {
        super.stop();

        rollService.shutdown();
    }

    String getCurrentIndexName(String serviceName) {
        Calendar cal = Calendar.getInstance();
        if (lastIndexRollingMinute.get(serviceName) != null &&
                lastIndexRollingMinute.get(serviceName).intValue() == (int)(System.currentTimeMillis() / 60000)) {
            //udpate indexType every minute
            return lastIndexRollingName.get(serviceName);
        }

        String currentIndexPrefix = "msec_" + serviceName;
        int    splitNum = 1;
        String splitUnit = "day";
        int pos = 0;
        while (pos < indexRollingTime.length() && Character.isDigit(indexRollingTime.charAt(pos)))
            ++pos;

        if (pos == 0) {
            splitNum = 1;
        } else if (pos < indexRollingTime.length()) {
            splitNum = Integer.valueOf(indexRollingTime.substring(0, pos).trim());
            splitUnit = indexRollingTime.substring(pos).trim();
        } else {
            splitNum = Integer.valueOf(indexRollingTime.trim());
        }
        if (splitNum == 0)  splitNum = 1;

        int month = cal.get(Calendar.MONTH) + 1;
        if (splitUnit.compareToIgnoreCase("day") == 0) {
            int dayOfMonth = cal.get(Calendar.DAY_OF_MONTH);
            if (dayOfMonth % splitNum == 0 || currentIndexName.isEmpty())
                currentIndexName = currentIndexPrefix + String.format("%02d%02d", month, dayOfMonth);
        } else if (splitUnit.compareToIgnoreCase("hour") == 0) {
            int dayOfMonth = cal.get(Calendar.DAY_OF_MONTH);
            int hourOfDay = cal.get(Calendar.HOUR_OF_DAY);
            if (hourOfDay % splitNum == 0 || currentIndexName.isEmpty())
                currentIndexName = currentIndexPrefix + String.format("%02d%02d%02d", month, dayOfMonth, hourOfDay);
        } else if (splitUnit.compareToIgnoreCase("min") == 0) {
            int dayOfMonth = cal.get(Calendar.DAY_OF_MONTH);
            int hourOfDay = cal.get(Calendar.HOUR_OF_DAY);
            int minute = cal.get(Calendar.MINUTE);
            if (hourOfDay % splitNum == 0 || currentIndexName.isEmpty())
                currentIndexName = currentIndexPrefix + String.format("%02d%02d%02d%02d",
                        month, dayOfMonth, hourOfDay, minute);
        }

        lastIndexRollingMinute.put(serviceName, (int)(System.currentTimeMillis() / 60000));
        lastIndexRollingName.put(serviceName, currentIndexName);
        return currentIndexName;
    }

    private void initESThreadPool() {
        serverAddresses = new InetSocketTransportAddress[serverAddressStrings.length];
        workingQueue = new ArrayBlockingQueue<ESClientThread.ESThreadRequest>(serverAddressStrings.length * 2);

        for (int i = 0; i < serverAddressStrings.length; i++) {
            String[] hostPort = serverAddressStrings[i].trim().split(":");
            String host = hostPort[0].trim();
            int port = hostPort.length == 2 ? Integer.parseInt(hostPort[1].trim()) : DEFAULT_PORT;
            try {
                serverAddresses[i] = new InetSocketTransportAddress(InetAddress.getByName(host), port);

                threadPool.submit(new ESClientThread(workingQueue, clusterName, serverAddresses[i]));
            } catch (UnknownHostException e) {
                e.printStackTrace();
            }
        }
    }

    public void submitESRequest(ESClientThread.ESThreadRequest request) {
        try {
            workingQueue.put(request);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        totalCount += request.sourceList.size() - 1;
        if (totalCount > 20000)
        {
            SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
            System.out.println(df.format(new Date()) + " totalCount: " + totalCount +
                    " cost: " + (System.currentTimeMillis() - timeStart) +
                    " qps: " + totalCount * 1000 / (System.currentTimeMillis() - timeStart));
            timeStart = System.currentTimeMillis();
            totalCount = 0;
        }
    }

    private void doSerialize(Event event) throws  IOException
    {
        Map<String, String>  headers = event.getHeaders();
        String content = null;
        String serviceName = "";

        if (!headers.containsKey("InsTime")) {
            long  insTime = System.currentTimeMillis();
            headers.put("InsTime", String.valueOf(insTime));
        }

        if (headers.containsKey("ServiceName")) {
            serviceName = headers.get("ServiceName");
            int pos = serviceName.indexOf(".");
            if (pos > 0) {
                serviceName = serviceName.substring(0, pos);
            }
        }

        ObjectMapper objectMapper = new ObjectMapper();
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        JsonGenerator jgen = null;
        try {
            jgen = objectMapper.getJsonFactory().createJsonGenerator(baos, JsonEncoding.UTF8);

            jgen.writeStartObject();
            for (String headerKey: headers.keySet()) {
                String headerValue = headers.get(headerKey);
                if (headerValue != null && !headerValue.isEmpty()) {
                    jgen.writeStringField(headerKey, headerValue);
                }
            }

            content = new String(event.getBody());
            content = content.replace('\t', ' ').replace('\n', ' ');
            if (content.length() > maxContentLength) {
                content = content.substring(0, maxContentLength - 15) + "<..truncated..>";
            }
            jgen.writeStringField("body", content);
            jgen.writeEndObject();
            jgen.flush();
            jgen = null;

            //outputStream.write((baos.toString() + "\n").getBytes());
            String curIndexName = getCurrentIndexName(serviceName.toLowerCase());
            //LOG.info("index: (" + curIndexName + "," + indexType + ") source: " + baos.toString());

            esThreadRequest.sourceList.add(baos.toString());
            esThreadRequest.indexNameList.add(curIndexName);
            esThreadRequest.indexTypeList.add(indexType);
            if (esThreadRequest.sourceList.size() >= bulkNum) {
                submitESRequest(esThreadRequest);
                esThreadRequest = new ESClientThread.ESThreadRequest();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        LOG.info("ES sink process: " + content + " " + headers.get("IP") + " " + headers.get("Level") + " " +headers.get("RPCName"));
    }

    @Override
    public Status process() throws EventDeliveryException {
        Status result = Status.READY;
        Channel channel = getChannel();
        Transaction transaction = channel.getTransaction();
        Event event;

        if (bulkTimeout) {
            bulkTimeout = false;

            if ( !esThreadRequest.sourceList.isEmpty() ) {
                LOG.info("ES bulk timeout");
                submitESRequest(esThreadRequest);
                esThreadRequest = new ESClientThread.ESThreadRequest();
            }
        }

        try {
            transaction.begin();
            for (int i = 0; i < batchSize; i++) {
                event = channel.take();
                if (event != null) {
                    doSerialize(event);
                } else {
                    // No events found, request back-off semantics from runner
                    result = Status.BACKOFF;
                    break;
                }
            }

            transaction.commit();
        } catch (Exception ex) {
            transaction.rollback();
            throw new EventDeliveryException("Failed to process transaction", ex);
        } finally {
            transaction.close();
        }

        return result;
    }


}


