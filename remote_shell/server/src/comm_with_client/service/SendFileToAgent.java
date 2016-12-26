
/*
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
package comm_with_client.service;

import comm_with_client.request.SendFileToAgentRequest;
import comm_with_client.response.SendFileToAgentResponse;
import ngse.remote_shell.JsonRPCHandler;
import ngse.remote_shell.Main;
import ngse.remote_shell.Tools;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.charset.Charset;
import java.security.MessageDigest;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Created by Administrator on 2016/2/4.
 */
public class SendFileToAgent extends JsonRPCHandler {


    private String geneSignature(String filename)
    {

        try {
            FileInputStream inputStream = new FileInputStream(filename);
            MessageDigest md = MessageDigest.getInstance("SHA-256");
            byte[] buf = new byte[10240];

            while (true) {
                int len = inputStream.read(buf);
                if (len <= 0)
                {
                    break;
                }

                md.update(buf,0, len);

            }
            byte[] result = md.digest();
            if (result.length != 32) {
                return "";
            }



            System.out.println("file sha256:"+Tools.toHexString(result));
            //combine file digest and current time together
            byte[] digestAndTimestamp = new byte[64];
            for (int i = 0; i < 32; i++) {
                digestAndTimestamp[i] = result[i];
            }
            byte[] currentSec = Tools.currentSeconds().getBytes(Charset.forName("UTF-8"));
            for (int i = 0; i < 32; i++) {
                digestAndTimestamp[i+32] = currentSec[i];
            }
            //encrypt
            result = Tools.encryptWithPriKey(digestAndTimestamp, Main.privateKey);
            if (result == null || result.length < 64)
            {
                return "";
            }
            return Tools.toHexString(result);

        }
        catch (Exception e )
        {
            e.printStackTrace();
            return "";
        }
    }

    private String doSendFileToAgent(SendFileToAgentRequest request)
    {
        Socket socket = new Socket();
        Logger logger = Logger.getLogger("remote_shell");
        try {
            File file = new File(request.getLocalFileFullName());

            //建立到agent的短连接
            socket.connect(new InetSocketAddress(request.getRemoteServerIP(), 9981), 1000);

            //对命令文件进行签名
            String signature = "";
            if (Main.privateKey != null)
            {
                signature = geneSignature(request.getLocalFileFullName());
            }

            // 请求包的json字符串部分
            String s = "{\"handleClass\":\"SendFileToAgent\", \"requestBody\":{"+
                    "\"fileFullName\":\""+request.getRemoteFileFullName()+
                    "\",\"fileLength\":"+String.format("%d", file.length())+
                    ",\"signature\":\""+signature+"\""+
                    "}}";
            s = Tools.getLengthField(s.length())+s;

            //发送json string
            socket.getOutputStream().write(s.getBytes());
            logger.info(String.format("write json complete.%s", s));


            //send file content
            FileInputStream fileInputStream = new FileInputStream(file);
            byte[] buf = new byte[10240];
            int total_len = 0;
            while (true) {
                int len = fileInputStream.read(buf);
                if (len <= 0)
                {
                    break;
                }
                total_len+= len;
                socket.getOutputStream().write(buf, 0, len);
            }
            logger.info(String.format("write file bytes number:%d", total_len));

            //关闭发送
            socket.shutdownOutput();


            //recv the response
            // json string length field,  10bytes
            total_len  = 0;
            while (total_len < 10)
            {
                int len = socket.getInputStream().read(buf, total_len, 10-total_len);
                if (len <= 0)
                {
                    return "agent response invalid";
                }
                total_len += len;
            }
            int jsonLen = new Integer(new String(buf, 0, total_len).trim()).intValue();
            logger.info(String.format("response json string len:%d", jsonLen));

            if (jsonLen > buf.length)
            {
                return "agent response json string is too long";
            }

            // recv json string
            total_len  = 0;
            while (jsonLen < buf.length && total_len < jsonLen)
            {
                int len = socket.getInputStream().read(buf, total_len, jsonLen-total_len);
                if (len <= 0)
                {
                    return "agent response json string is incomplete";
                }
                total_len += len;
            }
            logger.info(String.format("read agent response bytes number:%d", total_len));
            String jsonStr = new String(buf, 0, total_len);

            //analyze the response
            JSONObject jsonObject = new JSONObject(jsonStr);
            String message = jsonObject.getString("message");
            int status = jsonObject.getInt("status");
            if (status == 0)
            {
                return "success";
            }
            else
            {
                return message;
            }


        }
        catch (IOException e)
        {
            return e.toString();
        }
        finally {
            try {socket.close();} catch (Exception e){}
        }

    }
    public SendFileToAgentResponse exec(SendFileToAgentRequest request)
    {
        Logger logger = Logger.getLogger("remote_shell");
        logger.setLevel(Level.ALL);

        logger.info(String.format("I will send file to agent, [%s],[%s],[%s]", request.getLocalFileFullName(), request.getRemoteFileFullName(),
                request.getRemoteServerIP()));

        String resultStr = doSendFileToAgent(request);
        logger.info(String.format("doSendFileToAgent() result:%s", resultStr));
        SendFileToAgentResponse response = new SendFileToAgentResponse();
        if (resultStr.equals("success")) {
            response.setMessage("success");
            response.setStatus(0);
        }
        else
        {
            response.setMessage(resultStr);
            response.setStatus(100);
        }

        return response;
    }


}
