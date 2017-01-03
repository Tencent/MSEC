
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

import org.apache.log4j.Logger;
import org.codehaus.jackson.map.ObjectMapper;
import org.json.JSONObject;

import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.nio.charset.Charset;

/**
 * Created by Administrator on 2016/1/23.
 * ngse console这个项目的所有异步json请求都是由这个servlet来处理的
 * 该类主要是利用java反射机制，自动根据json请求里制定的handleClass来动态
 * 执行后台的java bean，准确的说是执行后台java bean的exec函数。并自动实现
 * json字符串与java类之间的序列化和反序列化
 */
@WebServlet(name = "JsonRPCServlet")
public class JsonRPCServlet extends HttpServlet {
    protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

    }

    protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

    }
    //应答一个包含错误信息的json字符串给前端
    private void errorResponse(HttpServletResponse resp, String message)
    {
        resp.setCharacterEncoding("UTF-8");
        resp.setContentType("application/json; charset=utf-8");

        Logger logger = Logger.getLogger(JsonRPCServlet.class);

        JsonRPCResponseBase r = new JsonRPCResponseBase();
        r.setMessage(message);
        r.setStatus(100);
        try {
            PrintWriter out =  resp.getWriter();
            String s = new ObjectMapper().writeValueAsString(r);
            logger.error(s);
            out.println(s);
        }
        catch (Exception e){e.printStackTrace();}

    }
    protected void service(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {

        Logger logger = Logger.getLogger(JsonRPCServlet.class);
        req.setCharacterEncoding("UTF-8");


        //从http请求里获取完整的json字符串
        //json字符串是http请求的一个form字段
        String request_string =  req.getParameter("request_string");

        int maxLen = request_string.length();
        if (maxLen > 10240) { maxLen = 10240;}
        logger.info("request_string:" + request_string.substring(0, maxLen));

        //ObjectManager和JSONObject两个都是 json与java类之间的转换工具
        ObjectMapper objectMapper = new ObjectMapper();


        JSONObject jsonObject = new JSONObject(request_string);
        String handleClassStr = jsonObject.getString("handleClass");
        JSONObject requestBodyObj = jsonObject.getJSONObject("requestBody");
        logger.info("requestBody:" + requestBodyObj.toString());

        try
        {
            //加载handleClass字段指定的类，并创建它的实例
            Class<?> clazz = Class.forName(handleClassStr);
            JsonRPCHandler handler = (JsonRPCHandler) clazz.newInstance();

            //将http的上下文信息传给该实例，以方便在业务逻辑处理的时候访问
            handler.setHttpRequest(req);
            handler.setHttpResponse(resp);
            handler.setServlet(this);

            //获取类的所有成员方法，遍历找到exec函数
            Method[] methods = clazz.getMethods();
            boolean execFound = false;
            for (int i = 0; i <methods.length ; i++) {

                if (methods[i].getName().equals("exec")) {
                    execFound = true;

                    //获取exec的参数列表，要求只能有一个参数
                    Class<?>[] paramTypes = methods[i].getParameterTypes();
                    if (paramTypes.length != 1)
                    {
                        errorResponse(resp, "handle class's exec() method's param number is not 1!");
                        return;
                    }
                    //将json字符串的requestBody部分映射到一个java类的实例，作为exec函数的参数
                    Object exec_request = objectMapper.readValue(requestBodyObj.toString(), paramTypes[0]);

                    //检查exec 函数的返回类型，要求继承自JsonRPCResponseBase
                    /*
                    Class<?> returnType = methods[i].getReturnType();
                    String superClass = returnType.getSuperclass().getName();
                    if (superClass == null || !superClass.equals("ngse.org.JsonRPCResponseBase"))
                    {
                        errorResponse(resp, "method exec() should return a class extending JsonRPCResponseBase.");

                        return;
                    }
                    */

                    //调用exec函数，如果返回非空，就序列化为json字符串返回
                    //特殊场景下，例如文件下载等，exec 会返回null，不需要给前端返回json字符串
                    try {
                        Object exec_result = methods[i].invoke(handler, exec_request);
                        if (exec_result != null) {
                            String s = objectMapper.writeValueAsString(exec_result);//�����ص�bean���л�Ϊjson�ַ���

                            resp.setCharacterEncoding("UTF-8");
                            resp.setContentType("application/json; charset=utf-8");
                            PrintWriter out =  resp.getWriter();

                            out.println(s);

                            maxLen = s.length();
                            if (maxLen > 10240) { maxLen = 10240;}
                            logger.info("response:" + s.substring(0, maxLen));
                            out.close();
                        }

                        return;


                    } catch (InvocationTargetException e) {
                        logger.error(e.getMessage());
                        e.printStackTrace();
                    }


                }
            }
            if (!execFound)
            {
               errorResponse(resp, "handleClass has NOT method named exec()");
                return;
            }

        } catch (ClassNotFoundException e)
        {
            e.printStackTrace();
            logger.error(e.getMessage());

           errorResponse(resp, "ClassNotFoundException:" + e.toString());

            return;
        } catch (InstantiationException e)
        {
            e.printStackTrace();
            logger.error(e.getMessage());

            errorResponse(resp, "InstantiationException:" + e.toString());
           ;
            return;
        } catch (IllegalAccessException e)
        {
            e.printStackTrace();
            logger.error(e.getMessage());

            errorResponse(resp, "IllegalAccessException:" + e.toString());

            return;
        }

    }



}
