
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
import beans.request.IPPortPair;
import beans.response.BusiLogFieldResponse;
import ngse.org.JsonRPCHandler;
import org.apache.log4j.Logger;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.OutputStream;
import java.lang.reflect.Array;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;

/**
 * Created by Administrator on 2016/2/29.
 */
public class QueryBusiLogField extends JsonRPCHandler {
    String log_srv_ip;
    int log_srv_port;

    private String doGetBusiLogField(BusiLogField request,  ArrayList<BusiLogField> fields)
    {
        Logger logger = Logger.getLogger(QueryBusinessLog.class);


        String sJson = String.format("{\"getFieldsReq\":{\"appName\":null}}");
        logger.info("to log srv:"+sJson);
        String lens = String.format("%10d", sJson.length());

        ArrayList<IPPortPair> ipPortPairs = QueryBusinessLog.getBusiLogSrvIPPort();
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
             logger.info(String.format("response json string len:%d", jsonLen));
            if (jsonLen > buf.length)
            {
                logger.error("response json is too long"+jsonLen);
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
            logger.info(String.format("read logserver response bytes number:%d", total_len));

            String jsonStr = new String(buf, 0, total_len);
            logger.info("from log srv:"+jsonStr);
            JSONObject jsonObject = new JSONObject(jsonStr);
            jsonObject = jsonObject.getJSONObject("getFieldsRsp");

            int status = jsonObject.getInt("ret");
            if (status != 0)
            {
                String message = jsonObject.getString("errmsg");
                logger.error("log server returns errcode, msg:"+message);
                return message;
            }
            JSONArray infoArray = jsonObject.getJSONArray("fieldsInfo");

            for (int i = 0; i < infoArray.length(); i++) {
                JSONObject info = infoArray.getJSONObject(i);
                BusiLogField field = new BusiLogField();
                field.setField_type(info.getString("field_type"));
                field.setField_name(info.getString("field_name"));

                logger.info(String.format("field%d:%s,%s", i, field.getField_name(),
                        field.getField_type()));

                fields.add(field);

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
    public BusiLogFieldResponse exec(BusiLogField request)
    {
        BusiLogFieldResponse response = new BusiLogFieldResponse();
        ArrayList<BusiLogField> fields = new ArrayList<BusiLogField>();
        String msg = doGetBusiLogField(request, fields);
        if (msg == null || !msg.equals("success"))
        {
            response.setMessage(msg);
            response.setStatus(100);
        }
        else
        {
            response.setField_list(fields);
            response.setMessage("success");
            response.setStatus(0);
        }



        return response;
    }
}
