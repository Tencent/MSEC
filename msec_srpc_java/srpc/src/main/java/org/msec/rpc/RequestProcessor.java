
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

import api.log.msec.org.AccessLog;
import api.monitor.msec.org.AccessMonitor;
import com.google.protobuf.MessageLite;
import org.apache.log4j.Logger;
import org.jboss.netty.channel.ChannelHandlerContext;

import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Callable;


public class RequestProcessor implements Callable<RpcResponse> {
    private static Logger log = Logger.getLogger(RequestProcessor.class.getName());

    private RpcContext  rpcContext;

    private static ThreadLocal<Map<String, Object>> threadContext = new ThreadLocal<Map<String, Object>>();
    public static void setThreadContext(String key, Object value) {
        Map<String, Object> context = threadContext.get();
        if (context == null) {
            context = new HashMap<String, Object>();
            threadContext.set(context);
        }
        context.put(key, value);
    }

    public static Object getThreadContext(String key) {
        Map<String, Object> context = threadContext.get();
        if (context == null) {
            return null;
        }
        return context.get(key);
    }

    public static void clearThreadContext() {
        Map<String, Object> context = threadContext.get();
        if (context != null) {
            context.clear();
        }
        threadContext.remove();
    }

    public RequestProcessor(ChannelHandlerContext ctx, RpcRequest request) {
        rpcContext = new RpcContext(request, ctx);
    }

    public void  processListCgi(RpcRequest rpcRequest, RpcResponse  rpcResponse) {
        StringBuilder  sb = new StringBuilder();
        Map<String, Map<String, List<ServiceFactory.ServiceMethodEntry>>>  serviceMethodMap = ServiceFactory.getServiceMethodMap();

        List<String>   serviceMethodList = new ArrayList<String>();
        sb.append("<html><head><title>MSEC SRPC TEST</title></head>\n<body>\n");
        sb.append("<script type=\"text/javascript\" src=\"http://code.jquery.com/jquery-latest.js\"></script>\n");
        for (String serviceName : serviceMethodMap.keySet()) {
            sb.append("\t<h1>" + serviceName + "</h1><br><br> \n");

            for (String methodName : serviceMethodMap.get(serviceName).keySet()) {
                sb.append("\t<h2>" + methodName + "</h2> \n");
                List<ServiceFactory.ServiceMethodEntry>   entries = serviceMethodMap.get(serviceName).get(methodName);
                sb.append("\t<h4>" + entries.get(0).getReturnType().getCanonicalName() + "  ");
                sb.append(entries.get(0).getMethod().getName() + "(");
                sb.append(entries.get(0).getParamType().getCanonicalName() + ")</h4><br> \n");

                serviceMethodList.add(serviceName + "." + methodName);
            }
        }

        sb.append("\n\t<form method=\"post\" action=\"invoke\" name=\"dataform\">\n" +
                "\tService: <select name=\"servicename\">\n");

        for (String serviceMethod : serviceMethodList) {
            sb.append("\t<option>" + serviceMethod + "</option>\n");
        }
        sb.append("\t</select><br>\n");

        sb.append("\tRequest: <textarea name=\"reqjson\" rows=\"5\" cols=\"40\"></textarea>  \n" +
                "\t<a href=\"javascript:;\" onclick=\"ajaxpost()\">Invoke</a><br>  \n" +
                "\tResponse: <textarea name=\"rspjson\" rows=\"5\" cols=\"40\"></textarea>  \n" +
                "\t</form>  \n" +
                "\t\n" +
                "\t<script type=\"text/javascript\">  \n" +
                "\tfunction ajaxpost(){  \n" +
                "\tvar f = document.dataform;   \n" +
                "\tvar req = f.reqjson.value;  \n" +
                "\tvar url = \"/invoke?methodName=\" + f.servicename.value;\n" +
                "\n" +
                "\t$.post(url, req,\n" +
                "\t  function(data){\n" +
                "\t\tf.rspjson.value = data;\n" +
                "\t  },\n" +
                "\t  \"text\");\n" +
                "\t}  \n" +
                "</script>\n");
        sb.append("</body>\n</html>");

        rpcResponse.setResultObj(sb.toString());
        rpcResponse.setErrno(0);
    }

    public RpcResponse call() throws Exception {
        setThreadContext("session", rpcContext);
        RpcRequest rpcRequest = rpcContext.getRequest();
        ChannelHandlerContext  channelHandlerContext = rpcContext.getChannelContext();
        RpcResponse rpcResponse = new RpcResponse();

        rpcResponse.setSerializeMode(rpcRequest.getSerializeMode());
        rpcResponse.setSeq(rpcRequest.getSeq());
        rpcResponse.setResultObj(null);
        rpcResponse.setError(null);

        if (rpcRequest.getException() != null) {
            //already exceptions for the request
            rpcResponse.setErrno(-1);  //invalid request
            rpcResponse.setError(rpcRequest.getException());
            channelHandlerContext.getChannel().write(rpcResponse);
            return null;
        }

        if (rpcRequest.getHttpCgiName() != null && rpcRequest.getHttpCgiName().compareToIgnoreCase("/list") == 0) {
            processListCgi(rpcRequest, rpcResponse);
            channelHandlerContext.getChannel().write(rpcResponse);
            return null;
        }

        //Map<String, String> headers = new HashMap<String, String>();
        //headers.put("Coloring", "1");
        //AccessLog.doLog(AccessLog.LOG_LEVEL_DEBUG, headers, "system rpc log. request: " + rpcRequest.getParameter());

        String serviceMethodName = rpcRequest.getServiceName() + "/" + rpcRequest.getMethodName();
        log.info("Rpc Call: " + rpcContext.getClientAddr() + " ---> " +
                rpcContext.getLocalAddr() + " " + serviceMethodName + " request: " + rpcRequest.getParameter());
        //AccessLog.doLog(AccessLog.LOG_LEVEL_INFO, "Rpc Call: " + rpcContext.getClientAddr() + " ---> " +
        //        rpcContext.getLocalAddr() + " " + serviceMethodName + " request: " + rpcRequest.getParameter());

        ServiceFactory.ServiceMethodEntry serviceMethodEntry = ServiceFactory.getServiceMethodEntry(rpcRequest.getServiceName(), rpcRequest.getMethodName());
        if (serviceMethodEntry != null) {
            Object invokeResultObj = null;
            try {
                long timeBegin = System.currentTimeMillis();
                invokeResultObj = serviceMethodEntry.invoke(rpcRequest.getParameter());
                long timeEnd = System.currentTimeMillis();
                log.error("Request processor time cost: " + (timeEnd - timeBegin));
                rpcResponse.setErrno(0);
                rpcResponse.setResultObj(invokeResultObj);

                AccessMonitor.add("frm.rpc " + rpcRequest.getMethodName() + " succ count");
                log.info("RPC invoke discrete method succeeded: " + invokeResultObj);
                //AccessLog.doLog(AccessLog.LOG_LEVEL_INFO, "Rpc Call timecost: " + (timeEnd - timeBegin) + "ms response: " + invokeResultObj);
            } catch (InvocationTargetException ex) {
                rpcResponse.setErrno(-1000);
                rpcResponse.setError(ex);
                log.error("Invoke method failed. " + ex.getMessage());
                AccessLog.doLog(AccessLog.LOG_LEVEL_ERROR, "Rpc Call exception: " + ex.getMessage());

                AccessMonitor.add("frm.rpc " + rpcRequest.getMethodName() + " failed count");
            } catch (IllegalAccessException ex) {
                rpcResponse.setErrno(-1000);
                rpcResponse.setError(ex);

                log.error("Invoke method failed. " + ex.getMessage());
                AccessLog.doLog(AccessLog.LOG_LEVEL_ERROR, "Rpc Call exception: " + ex.getMessage());
                AccessMonitor.add("frm.rpc " + rpcRequest.getMethodName() + " failed count");
            }
        } else {
            log.error("No service method registered: " + rpcRequest.getServiceName() + rpcRequest.getMethodName());
            AccessLog.doLog(AccessLog.LOG_LEVEL_ERROR, "Rpc Call exception: No service method registered: "
                    + rpcRequest.getServiceName() + rpcRequest.getMethodName());
            AccessMonitor.add("frm.rpc " + rpcRequest.getMethodName() + " not registered");
            AccessMonitor.add("frm.rpc " + rpcRequest.getMethodName() + " failed count");

            rpcResponse.setErrno(-1);
            rpcResponse.setError(new Exception("No service method registered: " +
                    rpcRequest.getServiceName() + rpcRequest.getMethodName()));
        }

        channelHandlerContext.getChannel().write(rpcResponse);
        return null;
    }
}