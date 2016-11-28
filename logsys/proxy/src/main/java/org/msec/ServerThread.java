
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


package org.msec;

import org.apache.log4j.Logger;
import org.codehaus.jackson.map.ObjectMapper;

import java.io.*;
import java.net.Socket;
import java.sql.SQLException;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

public class ServerThread extends Thread {

    static Logger logger = Logger.getLogger(ServerThread.class.getName());

    Socket socket = null;
    Properties props = null;
    String confFilename = null;
    byte[] buf = new byte[1024*1024];
    public ServerThread(Socket socket, String confFilename, Properties props) {
        this.socket = socket;
        this.confFilename = confFilename;
        this.props = props;
    }

    void sendReply(OutputStream os, String s) throws IOException {
        String len = String.format("%-10d", s.length());
        os.write(len.getBytes());
        os.write(s.getBytes());
    }

    int parseRequest(InputStream is) throws IOException {
        if (10 != is.read(buf, 0, 10)) {
            return -1;
        }

        int len = Integer.parseInt(new String(buf, 0, 10).trim());
        if (len != is.read(buf, 10, 10 + len)) {
            return -2;
        }

        return len;
    }

    //线程执行的操作，响应客户端的请求
    public void run() {
        InputStream is = null;

        OutputStream os = null;
        String line = null;
        String reqJsonStr = null;
        LogsysReq req;
        org.msec.LogsysRsp rsp;
        ObjectMapper objectMapper = new ObjectMapper();
        int len = 0;

        try {
            //获取一个输入流，并读取客户端的信息
            is = socket.getInputStream();
            //获取输出流，响应客户端的请求
            os = socket.getOutputStream();

            len = parseRequest(is);
            socket.shutdownInput(); //关闭输入流
            if (len <= 0) {
                socket.shutdownInput(); //关闭输入流
                logger.error("Invalid Request: parse request failed.");
                sendReply(os, "Invalid Request: parse request failed.");
                os.flush();  //将缓存输出
                return;
            }

            reqJsonStr = new String(buf, 10, 10 + len);
            logger.error("json parse:\n" + reqJsonStr);
            req = objectMapper.readValue(reqJsonStr, LogsysReq.class);
            if (req == null) {
                logger.error("Invalid Request: illegal json.");
                sendReply(os, "Invalid Request: illegal json.");
                os.flush();  //将缓存输出
                return;
            }

            logger.error("json parse ok.");

            try {
                rsp = processRequest(req);  //处理请求，将结果输出
                /*String logs = null;
                if (req.queryLogReq != null) {
                    logs = rsp.queryLogRsp.logs;
                    rsp.queryLogRsp.logs = "";
                }*/
                String retJsonStr = objectMapper.writeValueAsString(rsp);
                sendReply(os, retJsonStr);

                /*
                if (req.queryLogReq != null) {
                    sendReply(os, logs);
                }*/
                os.flush();
            } catch (Exception e) {
                e.printStackTrace();

                logger.error("System Failure.");
                sendReply(os, "System Failure.");
                os.flush();  //将缓存输出
            }
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                //关闭资源
                if (os != null)
                    os.close();
                if (is != null)
                    is.close();
                if (socket != null)
                    socket.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public org.msec.LogsysRsp processRequest(LogsysReq req) throws SQLException, ClassNotFoundException {
        org.msec.LogQuery logQuery = new org.msec.LogQuery(props.getProperty("hostname"),
                Integer.parseInt(props.getProperty("port")),
                props.getProperty("user"),
                props.getProperty("password"),
                props.getProperty("delfields"));
        org.msec.LogsysRsp rsp = new org.msec.LogsysRsp();

        if (req.queryLogReq != null) {
            logger.info(req.queryLogReq.toString());

            //处理日志查询请求

            long timeStart = System.currentTimeMillis();
            Map<String, String> headerFilters = new HashMap<String, String>();
            for (org.msec.LogField field : req.queryLogReq.getFilterFieldList())
                headerFilters.put(field.getField_name(), field.getField_value());
            if ( !req.queryLogReq.getAppName().isEmpty() ) {
                headerFilters.put("ServiceName", req.queryLogReq.getAppName());
            }
            rsp.queryLogRsp = logQuery.queryRecords(0, headerFilters, req.queryLogReq.maxRetNum,
                    req.queryLogReq.getStartDate(),  req.queryLogReq.getEndDate(),
                    req.queryLogReq.getStartTime(), req.queryLogReq.getEndTime(),
                    req.queryLogReq.getWhereCondition());

            long timeEnd = System.currentTimeMillis();
            logger.info("Rsp ret=" + rsp.queryLogRsp.getRet() + " lines=" + rsp.queryLogRsp.getLines() +
                    " errmsg=" + rsp.queryLogRsp.getErrmsg());
            logger.info("Timecost for query process: " + (timeEnd - timeStart) + "ms");
        } else if (req.modifyFieldsReq != null)  {
            logger.info(req.modifyFieldsReq);
            //处理修改字段请求
            rsp.modifyFieldsRsp = logQuery.modifyFields(req.modifyFieldsReq, confFilename, props);

            logger.info(rsp.modifyFieldsRsp);
        } else if (req.getFieldsReq != null)  {
            logger.info(req.getFieldsReq);
            //处理获取字段请求
            rsp.getFieldsRsp = logQuery.getFields(req.getFieldsReq);

            logger.info(rsp.getFieldsRsp);
        } else if (req.callGraphReq != null) {
            logger.info(req.callGraphReq);

            //处理调用关系图请求
            Map<String, String> headerFilters = new HashMap<String, String>();
            if (req.callGraphReq.getFilterFieldList() != null) {
                for (org.msec.LogField field : req.callGraphReq.getFilterFieldList())
                    headerFilters.put(field.getField_name(), field.getField_value());
            }
            if (req.callGraphReq.getReqId() != null && !req.callGraphReq.getReqId().isEmpty()) {
                headerFilters.put("ReqID", req.callGraphReq.getReqId());
            }
            if (req.callGraphReq.getAppName() != null &&  !req.callGraphReq.getAppName().isEmpty() ) {
                headerFilters.put("ServiceName", req.callGraphReq.getAppName());
            }

            long timeStart = System.currentTimeMillis();
            rsp.callGraphRsp = logQuery.callGraph(0, headerFilters, 1000,
                    req.callGraphReq.getStartDate(), req.callGraphReq.getEndDate(),
                    req.callGraphReq.getStartTime(), req.callGraphReq.getEndTime(),
                    null);

            long timeEnd = System.currentTimeMillis();
            logger.info("Rsp ret=" + rsp.callGraphRsp.getRet() + " graph=" + rsp.callGraphRsp.getGraph() +
                    " errmsg=" + rsp.callGraphRsp.getErrmsg());
            logger.info("Timecost for call graph process: " + (timeEnd - timeStart) + "ms");
        }

        logQuery.close();
        return rsp;
    }
}