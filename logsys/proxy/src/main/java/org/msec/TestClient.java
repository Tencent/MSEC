
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

/**
 * Created by elvinqiu on 2016/2/27.
 */

import org.apache.commons.cli.*;
import org.codehaus.jackson.map.ObjectMapper;

import java.io.*;
import java.net.Socket;

public class TestClient {

    public static void setRequest(org.msec.LogsysReq.QueryLogReq req)
    {
        req.setAppName("TestService");
        req.setLogLevel("ERROR");
        req.addFilterField(new LogField("RPCName", "", "rpc814"));
        req.setMaxRetNum(1000);
        req.setStartDate("2016-04-08");
        req.setEndDate("2016-04-08");
        req.setStartTime("00:00:00");
        req.setEndTime("23:59:00");
        req.setWhereCondition("");
    }

    public static void main(String[] args)  {


        Options options = new Options();

        Option option = new Option("t", "type", true, "the type of this request: query/add/del/get/graph");
        option.setRequired(true);
        options.addOption(option);

        option = new Option("f", "field", true,
                "required if add field or del field");
        option.setRequired(false);
        options.addOption(option);

        option = new Option("h", "help", false,
                "show help");
        options.addOption(option);

        CommandLineParser parser = new GnuParser();
        CommandLine commandLine = null;
        try {
            commandLine = parser.parse(options, args);
        } catch (ParseException e) {
            System.out.println("Parse command line failed.");
            e.printStackTrace();
            return;
        }

        if (commandLine.hasOption('h')) {
            new HelpFormatter().printHelp("LogsysProxy Client", options, true);
            return;
        }

        String reqType = commandLine.getOptionValue('t');
        String fieldName = commandLine.getOptionValue("f");
        if (!reqType.equals("query") && !reqType.equals("add")
            && !reqType.equals("del") && !reqType.equals("get")
            && !reqType.equals("graph")) {
            System.out.println("Invalid type " +reqType);
            return;
        }

        if ((reqType.equals("add") || reqType.equals("del"))
                && (fieldName == null || fieldName.isEmpty())) {
            System.out.println("No fieldname specified.");
            return;
        }

        try {
            Socket socket = new Socket("localhost", 30150);
            OutputStream os = socket.getOutputStream(); //字节输出流

            org.msec.LogsysReq req = new org.msec.LogsysReq();
            if (reqType.equals("query")) {
                req.queryLogReq = new org.msec.LogsysReq.QueryLogReq();
                setRequest(req.queryLogReq);
            }

            if (reqType.equals("add")) {
                req.modifyFieldsReq = new org.msec.LogsysReq.ModifyFieldsReq();
                req.modifyFieldsReq.setAppName("App");
                req.modifyFieldsReq.setFieldName(fieldName);
                req.modifyFieldsReq.setFieldType("String");
                req.modifyFieldsReq.setOperator("ADD");
            }

            if (reqType.equals("del")) {
                req.modifyFieldsReq = new org.msec.LogsysReq.ModifyFieldsReq();
                req.modifyFieldsReq.setAppName("TestApp");
                req.modifyFieldsReq.setFieldName(fieldName);
                req.modifyFieldsReq.setFieldType("String");
                req.modifyFieldsReq.setOperator("DEL");
            }

            if (reqType.equals("get")) {
                req.getFieldsReq = new org.msec.LogsysReq.GetFieldsReq();
                req.getFieldsReq.setAppName("TestApp");
            }

            if (reqType.equals("graph")) {
                req.callGraphReq = new org.msec.LogsysReq.CallGraphReq();
                req.callGraphReq.setAppName("JavaSample.Jecho");
                //req.callGraphReq.addFilterField(new LogField("RPCName", "", "rpc814"));
                req.callGraphReq.setStartDate("2016-07-04");
                req.callGraphReq.setEndDate("2016-07-04");
                req.callGraphReq.setStartTime("00:00:00");
                req.callGraphReq.setEndTime("23:59:00");
            }

            ObjectMapper objectMapper = new ObjectMapper();
            String reqJsonStr = objectMapper.writeValueAsString(req);
            String reqJsonLen = String.format("%-10d", reqJsonStr.length());

            System.out.println(reqJsonLen + reqJsonStr);
            os.write(reqJsonLen.getBytes());
            os.write(reqJsonStr.getBytes());
            socket.shutdownOutput(); //关闭输出流

            InputStream is = socket.getInputStream();

            byte[] buff = new byte[1024];
            int readLen = 0;
            //循环读取
            while ((readLen = is.read(buff)) > 0){
                System.out.println(new String(buff));
            }

            is.close();
            os.close();
            socket.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
