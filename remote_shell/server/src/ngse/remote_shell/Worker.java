
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


//该类描述了线程池里执行的任务
//入口函数是run()
package ngse.remote_shell;



import org.codehaus.jackson.map.ObjectMapper;
import org.json.JSONObject;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.net.Socket;

/**
 * Created by Administrator on 2016/2/4.
 */
public class Worker implements Runnable {
    private  Socket socket;//来自前端连接的socket

    public Worker(Socket s)
    {
        socket = s;
    }

    private void returnErrorMessage(String msg)
    {
        JsonRPCResponseBase r = new JsonRPCResponseBase();
        ObjectMapper objectMapper = new ObjectMapper();
        r.setMessage(msg);
        r.setStatus(100);
        try {
            String s = objectMapper.writeValueAsString(r);
            socket.getOutputStream().write(s.getBytes());
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return ;
        }

    }

    //利用反射机制，
    // 1.根据请求json字符串中的handleClass字段，实例化实际的处理类
    // 2. 自动将请求json字符串实例化为java类，相当于拆请求包
    // 3. 自动将处理后返回的java类实例，序列化为json 字符串，作为应答发送给前端
    private String handleRequest(String jsonStr, InputStream in, OutputStream out)
    {
        JSONObject jsonObject = new JSONObject(jsonStr);
        String handleClassStr = jsonObject.getString("handleClass");
        JSONObject requestBodyObj = jsonObject.getJSONObject("requestBody");
        ObjectMapper objectMapper = new ObjectMapper();
        try
        {
            Class<?> clazz = Class.forName(handleClassStr);
            //实例化handle class指定的类
            JsonRPCHandler handler = (JsonRPCHandler) clazz.newInstance();

            //获得该类的所有方法，找到exec方法并运行
            Method[] methods = clazz.getMethods();
            boolean execFound = false;
            for (int i = 0; i <methods.length ; i++) {

                if (methods[i].getName().equals("exec")) {
                    execFound = true;

                    //exec方法的参数的类型，将json反序列化为该参数的实例
                    Class<?>[] paramTypes = methods[i].getParameterTypes();
                    if (paramTypes.length != 1)
                    {
                        returnErrorMessage("handle class's exec() method's param number is not 1!");
                        return "failed";
                    }
                    Object exec_request = objectMapper.readValue(requestBodyObj.toString(), paramTypes[0]);

                    //检查exec方法的返回类型,要求继承自JsonRPCResponseBase
                    Class<?> returnType = methods[i].getReturnType();
                    String superClass = returnType.getSuperclass().getName();
                    if (superClass == null || !superClass.equals("ngse.remote_shell.JsonRPCResponseBase"))
                    {
                        returnErrorMessage("method exec() should return a class extending JsonRPCResponseBase.");
                        return "failed";
                    }

                    //运行exec方法,并将返回的java类实例序列化为json字符串，应答前端
                    try {
                        Object exec_result = methods[i].invoke(handler, exec_request);
                        if (exec_request != null) {
                            String s = objectMapper.writeValueAsString(exec_result);//???????bean???л??json?????
                            out.write(s.getBytes());
                        }

                        return "success";


                    } catch (InvocationTargetException e) {
                        e.printStackTrace();
                        return "failed";
                    }


                }
            }
            if (!execFound)
            {
                return "exec method not found";
            }

        } catch (ClassNotFoundException e)
        {
            e.printStackTrace();


            return "ClassNotFoundException:" + e.toString();
        } catch (InstantiationException e)
        {
            e.printStackTrace();

            return "InstantiationException:" + e.toString();
        } catch (IllegalAccessException e)
        {
            e.printStackTrace();

            return "IllegalAccessException:" + e.toString();
        }
        catch ( Exception e)
        {
            e.printStackTrace();

            return "IllegalAccessException:" + e.toString();
        }
        return "failed";

    }
    //从网络中读取一个完整的json字符串
    private String readJsonString(StringBuffer jsonStr, InputStream in)
    {
        byte[] buf = new byte[10240];

        try {
            /*
            //先读一下json字符串的长度 10字节的字符串
            sum = 0;
            while (sum < 10) {
                len = in.read(buf, sum, 10-sum);
                System.out.printf("read %d bytes\n", len);
                if (len <= 0) {
                    return "read length failed!";
                }
                sum += len;
            }
            int jsonLen = new Integer(new String(buf, 0, 10).trim()).intValue();
            //读json内容
            if (jsonLen > buf.length)
            {
                return "json string is too long";
            }
            sum = 0;
            while (sum < jsonLen)
            {
                len = in.read(buf, sum, buf.length-sum);
                System.out.printf("read %d bytes\n", len);
                if (len == -1)
                {
                    return "unexpected end of input";
                }
                sum += len;
            }

            jsonStr.append(new String(buf, 0, jsonLen));
            */

            //读，一直读到对端关闭写
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
            jsonStr.append(new String(buf, 0, offset));
            return "success";
        }
        catch (IOException e)
        {
            return e.toString();
        }

    }
    @Override
    public void run() {
        try
        {
            InputStream in = socket.getInputStream();
            OutputStream out = socket.getOutputStream();
            StringBuffer jsonStr = new StringBuffer();
            String resultStr = readJsonString(jsonStr, in);
            if (!resultStr.equals("success"))
            {
                System.out.println(resultStr);

                return;
            }
            System.out.println("json string:"+jsonStr.toString());
            resultStr = handleRequest(jsonStr.toString(), in, out);
            if (!resultStr.equals("success"))
            {
                System.out.println(resultStr);
                return;
            }
            return;
        }
        catch (IOException e)
        {
            e.printStackTrace();
            return;
        }
        finally {
            System.out.println("close the socket.");
            try     {   socket.close();} catch (Exception e){}
        }

    }
}
