
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

import beans.request.BusiLogField;
import beans.request.BusinessLog;
import beans.request.IPPortPair;
import beans.response.BusiLogFieldResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import org.apache.log4j.Logger;
import org.json.JSONObject;

import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.Charset;
import java.util.ArrayList;

/**
 * Created by Administrator on 2016/2/29.
 */
public class DeleteBusiLogField extends JsonRPCHandler {
    String log_srv_ip;
    int log_srv_port;

    private String doDeleteBusiLogField(BusiLogField request)
    {
        Logger logger = Logger.getLogger(QueryBusinessLog.class);


        String sJson = String.format("{\"modifyFieldsReq\":{\"appName\":null,  \"operator\":\"DEL\","+
                "\"fieldName\":\"%s\",\"fieldType\":\"%s\"}}",request.getField_name(), request.getField_type());
        logger.info("request to log server:"+sJson);
        String lens = String.format("%10d", sJson.length());

        ArrayList<IPPortPair> ipPortPairs = QueryBusinessLog.getBusiLogSrvIPPort();
        if (ipPortPairs == null || ipPortPairs.size() < 1)
        {
            return "get log server ip failed";
        }
        log_srv_ip = ipPortPairs.get(0).getIp();
        log_srv_port = ipPortPairs.get(0).getPort();

        logger.info(String.format("log server:%s:%d", log_srv_ip, log_srv_port));
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
            byte[] buf = new byte[10240];
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
            // logger.error(String.format("response json string len:%d", jsonLen));
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
            logger.info("log server resp:"+jsonStr);
            JSONObject jsonObject = new JSONObject(jsonStr);
            jsonObject = jsonObject.getJSONObject("modifyFieldsRsp");

            int status = jsonObject.getInt("ret");
            if (status != 0)
            {
                String message = jsonObject.getString("errmsg");
                return message;
            }

            return "success";
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
        finally {
            if (socket!=null) {
                try {socket.close();} catch (Exception e){};
            }
        }

    }
    public BusiLogFieldResponse exec(BusiLogField request)
    {
        BusiLogFieldResponse response = new BusiLogFieldResponse();
        String result = doDeleteBusiLogField(request);
        if (result == null || !result.equals("success")) {
            response.setMessage(result);
            response.setStatus(100);
        }
        else
        {
            response.setMessage("success");
            response.setStatus(0);
        }
        return response;
    }
}
