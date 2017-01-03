
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


package ngse.org;

import org.json.JSONObject;

import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;

/**
 * Created by Administrator on 2016/3/9.
 */
public class RemoteShell {

    private String serverIP = "127.0.0.1";
    private int serverPort = 9981;

    public RemoteShell()
    {

    }
     private String getLengthField(int len)
    {
        StringBuffer sb = new StringBuffer();
        sb.append(new Integer(len).toString());
        while (sb.length() < 10)
        {
            sb.append(" ");
        }
        return sb.toString();
    }
    public  String SendFileToAgent(String localFileFullName, String remoteFileFullName, String remoteServerIP)
    {
        Socket socket = new Socket();

        try {
            socket.setSoTimeout(60000);
            socket.connect(new InetSocketAddress(serverIP,serverPort), 5000);

            OutputStream out = socket.getOutputStream();
            InputStream in = socket.getInputStream();



            String request = "{\"handleClass\":\"comm_with_client.service.SendFileToAgent\", \"requestBody\":{"+
                    "\"localFileFullName\":\""+localFileFullName+
                    "\",\"remoteFileFullName\":\""+remoteFileFullName+
                    "\",\"remoteServerIP\":\""+remoteServerIP+
                    "\"}}";
           // request = getLengthField(request.length())+request;

           // System.out.printf("send:%s\n", request);
            out.write(request.getBytes());
            socket.shutdownOutput();

            //读，一直读到对端关闭写
            byte[] buf = new byte[10240];
            int offset = 0;
            while (true)
            {
                if (offset >= buf.length)
                {
                    return "buffer size is too small";
                }
                int len = in.read(buf, offset, buf.length-offset);
                if (len < 0)
                {
                    break;
                }
                offset += len;
            }
            String jsonStr = new String(buf, 0, offset);

            JSONObject obj = new JSONObject(jsonStr);
            int status = obj.getInt("status");
            if (status == 0)
            {
                return "success";
            }
            else
            {
                return obj.getString("message");
            }

        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
        finally {
            try {socket.close();}catch (Exception e){}
        }
    }
    public  String GetFileFromAgent(String localFileFullName,
                                            String remoteFileFullName,
                                            String remoteServerIP)
    {
        Socket socket = new Socket();

        try {
            socket.setSoTimeout(60000);
            socket.connect(new InetSocketAddress(serverIP,serverPort), 5000);

            OutputStream out = socket.getOutputStream();
            InputStream in = socket.getInputStream();




            String request = "{\"handleClass\":\"comm_with_client.service.GetFileFromAgent\", \"requestBody\":{\"localFileFullName\":\""+localFileFullName+
                    "\",\"remoteFileFullName\":\""+remoteFileFullName+
                    "\",\"remoteServerIP\":\""+remoteServerIP+
                    "\"}}";
           // request = getLengthField(request.length())+request;
           // System.out.printf("send:%s\n", request);
            out.write(request.getBytes());
            socket.shutdownOutput();

            //读，一直读到对端关闭写
            byte[] buf = new byte[10240];
            int offset = 0;
            while (true)
            {
                if (offset >= buf.length)
                {
                    return "buffer size is too small";
                }
                int len = in.read(buf, offset, buf.length-offset);
                if (len < 0)
                {
                    break;
                }
                offset += len;
            }
            String jsonStr = new String(buf, 0, offset);
            //检查结果
            JSONObject obj = new JSONObject(jsonStr);
            int status = obj.getInt("status");
            if (status == 0)
            {
                return "success";
            }
            else
            {
                return obj.getString("message");
            }

        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
        finally {
            try {socket.close();}catch (Exception e){}
        }
    }
    public String SendCmdsToRunAndGetResultBack(String cmd, String remoteServerIP, StringBuffer resultBack)
    {
        try
        {
            File cmdFile = File.createTempFile("cmd", ".sh");
            FileOutputStream out = new FileOutputStream(cmdFile);
            out.write(cmd.getBytes());
            out.close();

            StringBuffer outFileName = new StringBuffer();
            String result = SendCmdsToAgentAndRun(cmdFile.getAbsolutePath(), remoteServerIP, outFileName);
            if (result == null || !result.equals("success"))
            {
                return result;
            }
            File resultFile = File.createTempFile("result", ".txt");
            String localFile = resultFile.getAbsolutePath();
            result = GetFileFromAgent(localFile, outFileName.toString(), remoteServerIP);
            if (result == null || !result.equals("success"))
            {
                return result;
            }
            FileInputStream in  = new FileInputStream(localFile);
            byte[] buffer = new byte[10240];
            resultBack.delete(0, resultBack.length());
            while (true)
            {
                int len = in.read(buffer);
                if (len <= 0)
                {
                    break;
                }
                resultBack.append(new String(buffer,0,len));//不知道这样对于GBK被截断会不会有乱码...
            }
            in.close();

        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
        finally {

        }

        return "success";

    }
    public  String SendCmdsToAgentAndRun(String localFileFullName, String remoteServerIP, StringBuffer outputFileName )
    {
        Socket socket = new Socket();

        try {
            socket.setSoTimeout(60000);
            socket.connect(new InetSocketAddress(serverIP,serverPort), 5000);

            OutputStream out = socket.getOutputStream();
            InputStream in = socket.getInputStream();




            String request = "{\"handleClass\":\"comm_with_client.service.SendCmdsToAgentAndRun\", \"requestBody\":{\"localFileFullName\":\""+localFileFullName+
                    "\",\"remoteServerIP\":\""+remoteServerIP+
                    "\"}}";
           // request = getLengthField(request.length())+request;
           // System.out.printf("send:%s\n", request);
            out.write(request.getBytes());
            socket.shutdownOutput();

            //读，一直读到对端关闭写
            byte[] buf = new byte[10240];
            int offset = 0;
            while (true)
            {
                if (offset >= buf.length)
                {
                    return "buffer size is too small";
                }
                int len = in.read(buf, offset, buf.length-offset);
                if (len < 0)
                {
                    break;
                }
                offset += len;
            }
            String jsonStr = new String(buf, 0, offset);
            //检查结果
            JSONObject obj = new JSONObject(jsonStr);
            int status = obj.getInt("status");
            if (status == 0)
            {
                outputFileName.append(obj.getString("outputFileName"));
                return "success";
            }
            else
            {
                return obj.getString("message");
            }


        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
        finally {
            try {socket.close();}catch (Exception e){}
        }
    }


}
