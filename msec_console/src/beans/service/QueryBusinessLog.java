
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


package beans.service;

import beans.dbaccess.BusinessLogResult;
import beans.dbaccess.OddSecondLevelServiceIPInfo;
import beans.request.BusinessLog;
import beans.request.IPPortPair;

import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.JsonRPCResponseBase;
import ngse.org.ServletConfig;
import org.apache.log4j.Logger;
import org.graphviz.Graphviz;
import org.json.JSONArray;
import org.json.JSONObject;

import javax.servlet.ServletOutputStream;
import java.io.File;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.HashMap;


/**
 * Created by Administrator on 2016/2/27.
 */
public class QueryBusinessLog extends JsonRPCHandler {

    public static final String BUSI_LOG_RESULT_IN_SESSION = "BusinessLogResult";

    String log_srv_ip;
    int log_srv_port;
    public  static ArrayList<IPPortPair>  getBusiLogSrvIPPort()
    {
        DBUtil util = new DBUtil();
        try
        {
            if (util.getConnection() == null)
            {
                return null;
            }
            String sql =sql = "select ip,port from t_second_level_service_ipinfo where second_level_service_name='log' " +
                    "and first_level_service_name='RESERVED' and status='enabled'";
            ArrayList<IPPortPair>  list = util.findMoreRefResult(sql, null, IPPortPair.class);
            return list;
        }
        catch (Exception e )
        {
            e.printStackTrace();
            return null;
        }
        finally {
            util.releaseConn();
        }
    }
    public static String getGraphFilename(String reqid)
    {
        return ServletConfig.fileServerRootDir + "/tmp/"+reqid+".png";
    }
    //获取调用关系图
    private String doGetCallRelationGraph( BusinessLog request)
    {
        Logger logger = Logger.getLogger(QueryBusinessLog.class);



        String dt;
        dt = request.getDt_begin();
        if (dt == null || dt.length() < 14) { return "time field invalid";}
        String begin_dt = dt.substring(0, 4)+"-"+dt.substring(4,6)+"-"+dt.substring(6,8);
        String begin_tm = dt.substring(9);

        dt = request.getDt_end();
        if (dt == null || dt.length() < 14) { return "time field invalid";}
        String end_dt = dt.substring(0, 4)+"-"+dt.substring(4,6)+"-"+dt.substring(6,8);
        String end_tm = dt.substring(9);


        String ReqID = request.getRequest_id();
        String sJson = String.format("{\"callGraphReq\":{\"reqId\":\"%s\", \"filterFieldList\": null,  \"startDate\":\"%s\","+
         " \"endDate\":\"%s\", \"startTime\":\"%s\", \"endTime\":\"%s\"}}",  ReqID, begin_dt, end_dt, begin_tm, end_tm);
        logger.info("request to  log server:"+sJson);
        String lens = String.format("%10d", sJson.length());

        ArrayList<IPPortPair> ipPortPairs = getBusiLogSrvIPPort();
        if (ipPortPairs == null || ipPortPairs.size() < 1)
        {
            return "get log server ip failed";
        }
        log_srv_ip = ipPortPairs.get(0).getIp();
        log_srv_port = ipPortPairs.get(0).getPort();
        Socket socket = new Socket();
        try {
            socket.setSoTimeout(5000);
            socket.connect(new InetSocketAddress(log_srv_ip, log_srv_port), 3000);
            OutputStream outputStream = socket.getOutputStream();
            outputStream.write(lens.getBytes());
            outputStream.write(sJson.getBytes());
            socket.shutdownOutput();

            //收应答包
            // length field 10bytes
            byte[] buf = new byte[1024*1024];
            int total_len  = 0;
            while (total_len < 10)
            {
                int len = socket.getInputStream().read(buf, total_len, 10-total_len);
                if (len <= 0)
                {
                    return "receive json len failed.";
                }
                total_len += len;
            }
            int jsonLen = new Integer(new String(buf, 0, total_len).trim()).intValue();
            logger.info(String.format("response json string len:%d", jsonLen));
            if (jsonLen > buf.length)
            {
                return "response json is too long";
            }

            total_len  = 0;
            while ( total_len < jsonLen)
            {
                int len = socket.getInputStream().read(buf, total_len, jsonLen-total_len);
                if (len <= 0)
                {
                   return "receive json string failed.";
                }
                total_len += len;
            }


            String jsonStr = new String(buf, 0, total_len);


            logger.info("the length of log json string:"+jsonStr.length());


            logger.info("log resp:"+jsonStr);
            JSONObject jsonObject = new JSONObject(jsonStr);
            jsonObject = jsonObject.getJSONObject("callGraphRsp");

            int status = jsonObject.getInt("ret");
            if (status != 0)
            {
                String message = jsonObject.getString("errmsg");
                return message;
            }

            String dotLang = jsonObject.getString("graph");
            if ( !(dotLang == null || dotLang.equals("")) ) {
                String filename = getGraphFilename(ReqID);
                Graphviz.drawGraph(dotLang, filename);


                BusinessLogResult businessLogResult = (BusinessLogResult) (this.getHttpRequest().getSession().getAttribute(BUSI_LOG_RESULT_IN_SESSION));
                if (businessLogResult == null) {
                    return "failed to get log result from session.";
                }
                businessLogResult.setCall_relation_graph(ReqID);
            }

            return "success";
        }
        catch (Exception e)
        {
            e.printStackTrace();
            logger.error(e.getMessage());
            return e.getMessage();
        }
        finally {
            if (socket!=null) {
                try {socket.close();} catch (Exception e){};
            }
        }

    }
    private String doQueryLog( BusinessLog request)
    {
        Logger logger = Logger.getLogger(QueryBusinessLog.class);

        String svcname = request.getService_name();
        //if (svcname == null || svcname.length() < 1) { return "svcname field invalid";}
        String dt;
        dt = request.getDt_begin();
        if (dt == null || dt.length() < 14) { return "time field invalid";}
        String begin_dt = dt.substring(0, 4)+"-"+dt.substring(4,6)+"-"+dt.substring(6,8);
        String begin_tm = dt.substring(9);

        dt = request.getDt_end();
        if (dt == null || dt.length() < 14) { return "time field invalid";}
        String end_dt = dt.substring(0, 4)+"-"+dt.substring(4,6)+"-"+dt.substring(6,8);
        String end_tm = dt.substring(9);

        String where = "";
        if (request.getRequest_id() != null && request.getRequest_id().length() > 0)
        {
            if (where.length() > 0) { where += " and ";}
            where += " ReqID='"+request.getRequest_id()+"'";
        }
        if (request.getLog_ip() != null && request.getLog_ip().length() > 0)
        {
            if (where.length() > 0) { where += " and ";}
            where += " LocalIP='"+request.getLog_ip()+"' ";
        }
        if (request.getMore_condition() != null && request.getMore_condition().length() > 0)
        {
            if (where.length() > 0) { where += " and ";}
            where += request.getMore_condition();
        }
        String sJson = String.format("{\"queryLogReq\":{\"appName\":\"%s\",  \"logLevel\":\"DEBUG\","+
                "\"filterFieldList\": null,\"maxRetNum\":1000, \"startDate\":\"%s\","+
                " \"endDate\":\"%s\", \"startTime\":\"%s\", \"endTime\":\"%s\", "+
                "\"whereCondition\":\"%s\"}}", svcname, begin_dt, end_dt, begin_tm, end_tm, where);
        logger.info("request to  log server:"+sJson);
        String lens = String.format("%10d", sJson.length());

        ArrayList<IPPortPair> ipPortPairs = getBusiLogSrvIPPort();
        if (ipPortPairs == null || ipPortPairs.size() < 1)
        {
            return "get log server ip failed";
        }
        log_srv_ip = ipPortPairs.get(0).getIp();
        log_srv_port = ipPortPairs.get(0).getPort();
        Socket socket = new Socket();
        try {
            socket.setSoTimeout(5000);
            socket.connect(new InetSocketAddress(log_srv_ip, log_srv_port), 3000);
            OutputStream outputStream = socket.getOutputStream();
            outputStream.write(lens.getBytes());
            outputStream.write(sJson.getBytes());
            socket.shutdownOutput();

            //收应答包
            // length field 10bytes
            byte[] buf = new byte[1024*1024];
            int total_len  = 0;
            while (total_len < 10)
            {
                int len = socket.getInputStream().read(buf, total_len, 10-total_len);
                if (len <= 0)
                {
                    return "receive json len failed.";
                }
                total_len += len;
            }
            int jsonLen = new Integer(new String(buf, 0, total_len).trim()).intValue();
            logger.info(String.format("response json string len:%d", jsonLen));
            if (jsonLen > buf.length)
            {
                return "response json is too long";
            }

            total_len  = 0;
            while ( total_len < jsonLen)
            {
                int len = socket.getInputStream().read(buf, total_len, jsonLen-total_len);
                if (len <= 0)
                {
                    return "receive json string failed.";
                }
                total_len += len;
            }


            String jsonStr = new String(buf, 0, total_len);
            //String jsonStr = "{'queryLogRsp':{'ret':0, 'errmsg':'ok', 'records':[ ['b\"i{e:r}s\\'on', '1122[8]490', '36岁'],['babamama', '2202020302', '37岁']  ],heads:['name', 'uin', 'age']}}";

            logger.info("the length of log json string:"+jsonStr.length());


            // logger.info("log resp:"+jsonStr);
            JSONObject jsonObject = new JSONObject(jsonStr);
            jsonObject = jsonObject.getJSONObject("queryLogRsp");

            int status = jsonObject.getInt("ret");
            if (status != 0)
            {
                String message = jsonObject.getString("errmsg");
                return message;
            }

            ArrayList<String> columns = new ArrayList<String>();
            ArrayList<HashMap<String, String>> record_map_list = new ArrayList<HashMap<String, String>>();

            //返回的json字符串
            // ｛queryLogRsp：{ret:0, errmsg:'ok', records:[ ['fieldvalue1', 'fieldvalue2', 'fieldvalue3'], ...  ],heads:['ReqID', 'instime', ....]} ｝
            // 注意：引号要转义
            // 例如：{'queryLogRsp':{'ret':0, 'errmsg':'ok', 'records':[ ['b"i{e:r}s\'on', '1122[8]490', '36岁'],['babamama', '2202030202', '37岁']  ],heads:['name', 'uin', 'age']}}
            // json array里元素的顺序是否在序列化前后或者反序列化前后保持不变呢？
            JSONArray heads = jsonObject.getJSONArray("heads");
            StringBuffer logstr = new StringBuffer();
            for (int i = 0; i < heads.length(); i++) {
                columns.add(heads.getString(i));
                logstr.append(heads.getString(i));
                logstr.append(";");
            }
            logger.info("log head:"+logstr);

            JSONArray records = jsonObject.getJSONArray("records");
            for (int i = 0; i < records.length(); ++i)
            {
                JSONArray values = records.getJSONArray(i);
                if (values.length() != columns.size())
                {
                    return "data returned by log serv is invalid.";
                }
                HashMap<String, String> record_map = new HashMap<String, String>();
                for (int j = 0; j < values.length(); j++) {
                    String value = values.getString(j);

                    record_map.put(columns.get(j), value);
                }
                record_map_list.add(record_map);


            }
            logger.info("log record number:"+record_map_list.size());

            BusinessLogResult businessLogResult = new BusinessLogResult();
            businessLogResult.setColumn_names(columns);
            businessLogResult.setLog_records(record_map_list);

            this.getHttpRequest().getSession().setAttribute(BUSI_LOG_RESULT_IN_SESSION, businessLogResult );
            return "success";
        }
        catch (Exception e)
        {
            e.printStackTrace();
            logger.error(e.getMessage());
            return e.getMessage();
        }
        finally {
            if (socket!=null) {
                try {socket.close();} catch (Exception e){};
            }
        }

    }
    public JsonRPCResponseBase exec(BusinessLog request)
    {

        try {
            /*
            getHttpResponse().setContentType("text/html");
            getHttpResponse().setCharacterEncoding("UTF-8");
            ServletOutputStream out = getHttpResponse().getOutputStream();

            out.println("<html>");
            out.println("<head>");
            out.println("<title>log result</title>");
            out.println("</head>");
            out.println("<body>");
            out.println("<pre>");//这个标签用于原样输出

            String result = doQueryLog(out, request);
            if (result == null || !result.equals("success"))
            {
                out.println(result);
            }

            out.println("</pre>");
            out.println("</body>");
            out.println("</html>");


            out.close();
            return null;
            */

            JsonRPCResponseBase response = new JsonRPCResponseBase();

            String result = checkIdentity();
            if (!result.equals("success"))
            {

                response.setStatus(99);
                response.setMessage(result);
                return response;
            }


            result = doQueryLog(request);
            if ( !result.equals("success"))
            {
                response.setMessage(result);
                response.setStatus(100);
                return response;
            }
            String ReqID =  request.getRequest_id();
            if (ReqID != null && ReqID.length() > 0)
            {
                result = doGetCallRelationGraph(request);
                if ( !result.equals("success"))
                {
                    response.setMessage(result);
                    response.setStatus(100);
                    return response;
                }
            }




            response.setMessage("success");
            response.setStatus(0);
            return response;

        }
        catch (Exception e)
        {
            e.printStackTrace();
            return  null;
        }
    }
}
