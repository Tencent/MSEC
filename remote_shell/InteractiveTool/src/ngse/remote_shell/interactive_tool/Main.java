
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


package ngse.remote_shell.interactive_tool;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;



public class Main {

    static private String getLengthField(int len)
    {
        StringBuffer sb = new StringBuffer();
        sb.append(new Integer(len).toString());
        while (sb.length() < 10)
        {
            sb.append(" ");
        }
        return sb.toString();
    }
    static private void showUsage()
    {
        System.out.println("");
        System.out.println("1:SendFileToAgent");
        System.out.println("2:GetFileFromAgent");
        System.out.println("3:SendCmdsToAgentAndRun");
        System.out.println("h:show this help message");
        System.out.println("q:exit");
        System.out.print("Input the action number[1-2]:");
    }
    static private char getCmdChoice(String msg)
    {
        System.out.print(msg);
        try
        {
            byte[] buf = new byte[100];
            int len = System.in.read(buf);
            if (len < 1)
            {
                return 'h';
            }
            return new String(buf, 0, len).charAt(0);
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return 'h';
        }
    }
    static private int getIntFromStdin(String msg, int defaultVal)
    {
        System.out.print(msg);
        try
        {
            byte[] buf = new byte[100];
            int len = System.in.read(buf);
            if (len > 0 && buf[len-1] == "\n".getBytes()[0])
            {
                len--;
            }
            if (len > 0 && buf[len-1] == "\r".getBytes()[0])
            {
                len--;
            }
            if (len < 1)
            {
                return defaultVal ;
            }

            return  new Integer(new String(buf, 0, len)).intValue();
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return defaultVal;
        }
    }
    static private String getStringFromStdin(String msg, String defaultVal)
    {
        System.out.print(msg);
        try
        {
            byte[] buf = new byte[100];
            int len = System.in.read(buf);
            if (len > 0 && buf[len-1] == "\n".getBytes()[0])
            {
                len--;
            }
            if (len > 0 && buf[len-1] == "\r".getBytes()[0])
            {
                len--;
            }
            if (len < 1)
            {
                return defaultVal ;
            }

            return  new String(buf, 0, len);
        }
        catch(Exception e)
        {
            e.printStackTrace();
            return defaultVal;
        }
    }
    private static void cmdSendFileToAgent()
    {
        try {
            Socket socket = new Socket("localhost", 9981);
            OutputStream out = socket.getOutputStream();
            InputStream in = socket.getInputStream();

            String localFileFullName = null;
            String remoteFileFullName = null;
            String remoteServerIP = null;
            while (true) {
                localFileFullName = getStringFromStdin("the local file full name:", "");
                if (!localFileFullName.equals("")) {
                    break;
                }
            }
            while (true) {
                remoteFileFullName = getStringFromStdin("the remote file full name:", "");
                if (!remoteFileFullName.equals("")) {
                    break;
                }
            }
            while (true) {
                remoteServerIP = getStringFromStdin("the remote server IP:", "");
                if (!remoteServerIP.equals("")) {
                    break;
                }
            }

            String request = "{\"handleClass\":\"comm_with_client.service.SendFileToAgent\", \"requestBody\":{"+
                    "\"localFileFullName\":\""+localFileFullName+
                    "\",\"remoteFileFullName\":\""+remoteFileFullName+
                    "\",\"remoteServerIP\":\""+remoteServerIP+
                    "\"}}";
          //  request = getLengthField(request.length())+request;
            System.out.printf("send:%s\n", request);
            out.write(request.getBytes());
            socket.shutdownOutput();

            byte[] buf = new byte[1024];
            int len = in.read(buf);
            if (len > 0)
            {
                System.out.printf("recv:%s\n", new String(buf, 0, len));
            }
            socket.close();

        }
        catch (Exception e)
        {

        }
    }
    private static void cmdGetFileFromAgent()
    {
        try {
            Socket socket = new Socket("localhost", 9981);
            OutputStream out = socket.getOutputStream();
            InputStream in = socket.getInputStream();

            String localFileFullName = null;
            String remoteFileFullName = null;
            String remoteServerIP = null;
            while (true) {
                localFileFullName = getStringFromStdin("the local file full name:", "");
                if (!localFileFullName.equals("")) {
                    break;
                }
            }
            while (true) {
                remoteFileFullName = getStringFromStdin("the remote file full name:", "");
                if (!remoteFileFullName.equals("")) {
                    break;
                }
            }
            while (true) {
                remoteServerIP = getStringFromStdin("the remote server IP:", "");
                if (!remoteServerIP.equals("")) {
                    break;
                }
            }


            String request = "{\"handleClass\":\"comm_with_client.service.GetFileFromAgent\", \"requestBody\":{\"localFileFullName\":\""+localFileFullName+
                    "\",\"remoteFileFullName\":\""+remoteFileFullName+
                    "\",\"remoteServerIP\":\""+remoteServerIP+
                    "\"}}";
            //request = getLengthField(request.length())+request;
            System.out.printf("send:%s\n", request);
            out.write(request.getBytes());
            socket.shutdownOutput();

            byte[] buf = new byte[1024];
            int len = in.read(buf);
            if (len > 0)
            {
                System.out.printf("recv:%s\n", new String(buf, 0, len));
            }
            socket.close();

        }
        catch (Exception e)
        {

        }
    }
    private static void cmdSendCmdsToAgentAndRun()
    {
        try {
            Socket socket = new Socket("localhost", 9981);
            OutputStream out = socket.getOutputStream();
            InputStream in = socket.getInputStream();

            String localFileFullName = null;
            String remoteServerIP = null;

            while (true) {
                localFileFullName = getStringFromStdin("the local cmd file full name:", "");
                if (!localFileFullName.equals("")) {
                    break;
                }
            }
            while (true) {
                remoteServerIP = getStringFromStdin("the remote server IP:", "");
                if (!remoteServerIP.equals("")) {
                    break;
                }
            }



            String request = "{\"handleClass\":\"comm_with_client.service.SendCmdsToAgentAndRun\", \"requestBody\":{\"localFileFullName\":\""+localFileFullName+
                    "\",\"remoteServerIP\":\""+remoteServerIP+
                     "\"}}";
           // request = getLengthField(request.length())+request;
            System.out.printf("send:%s\n", request);
            out.write(request.getBytes());
            socket.shutdownOutput();

            byte[] buf = new byte[1024];
            int len = in.read(buf);
            if (len > 0)
            {
                System.out.printf("recv:%s\n", new String(buf, 0, len));
            }
            socket.close();

        }
        catch (Exception e)
        {

        }
    }

    public static void main(String[] args) {

        showUsage();
        while (true) {
            char cmd = getCmdChoice("");
            switch (cmd) {
                case '1':
                    cmdSendFileToAgent();
                    break;
                case '2':
                    cmdGetFileFromAgent();
                    break;
                case '3':
                    cmdSendCmdsToAgentAndRun();
                    break;
                case 'q':
                    return;
                default:
                    showUsage();
                    ;
                    break;
            }
        }



    }
}
