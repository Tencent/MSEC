
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

import beans.request.DevPackage;
import beans.request.ReleasePlan;

import beans.response.ReleaseStepsGOResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.JsonRPCResponseBase;
import org.apache.log4j.Logger;
import org.codehaus.jackson.map.ObjectMapper;
import sun.rmi.runtime.Log;

import javax.servlet.ServletOutputStream;
import javax.servlet.http.HttpSession;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Created by Administrator on 2016/2/1.
 * 下载开发包的每个步骤的处理
 */
public class DownloadDevGO extends JsonRPCHandler  {


    private JsonRPCResponseBase doStep1(DevPackage request)
    {
        JsonRPCResponseBase response = new JsonRPCResponseBase();
        //把计划保存到session里
        HttpSession session = getHttpRequest().getSession();

        DBUtil util = new DBUtil();
        try {
            util.getConnection();
            String sql = "select dev_lang from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
            ArrayList<Object> params = new ArrayList<Object>();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());

            Map<String, Object> map = util.findSimpleResult(sql, params);
            request.setDev_lang((String)map.get("dev_lang"));

        }
        catch (Exception e)
        {
            request.setDev_lang("c++");
        }
        finally {
            util.releaseConn();
        }


        session.setAttribute("DevPackage", request);

        response.setMessage("success");
        response.setStatus(0);
        return response;
    }

    private JsonRPCResponseBase doStep2(DevPackage request)
    {
        JsonRPCResponseBase response = new JsonRPCResponseBase();

        Logger logger = Logger.getLogger(DownloadDevGO.class);

        HttpSession session = getHttpRequest().getSession();
        DevPackage plan = (DevPackage)session.getAttribute("DevPackage");
        if (plan == null)
        {
            plan = new DevPackage();
            session.setAttribute("DevPackage", plan);
        }





        //制作tar包，这里还是同步的在当前线程完成，必要的话可以用另外一个
        //线程异步的完成。
        logger.info("begin making dev package...");
        PackDevFile packDevFile = new PackDevFile(plan, getServlet().getServletContext());
        packDevFile.run();
        if (!packDevFile.getResultString().equals("success"))
        {
            response.setMessage(packDevFile.getResultString());
            response.setStatus(100);
            return response;
        }
        String fileName = packDevFile.getOutputFileName();
        logger.info("dev package path:"+fileName);
        File f = new File(fileName);
        String length = String.format("%d", f.length());

        //将文件内容直接返回，注意MIME 类型，且exec函数应该返回null
        getHttpResponse().setHeader("Content-disposition", "attachment;filename="+new File(fileName).getName());
        // set the MIME type.
        getHttpResponse().setContentType("application/x-gzip-compressed");
        getHttpResponse().setHeader("Content_Length", length);

        try {
            ServletOutputStream out = getHttpResponse().getOutputStream();
            FileInputStream fileInputStream = new FileInputStream(f);
            byte[] buf = new byte[10240];
            while (true)
            {
                int len = fileInputStream.read(buf);
                if (len <= 0)
                {
                    break;
                }
                out.write(buf,0, len);
            }
            out.flush();
            out.close();

            fileInputStream.close();

            f.delete();
            return null;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return null;
        }


    }




    public JsonRPCResponseBase exec(DevPackage request)
    {
        JsonRPCResponseBase response = new JsonRPCResponseBase();
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getSecond_level_service_name() == null || request.getSecond_level_service_name().length() < 1||
            request.getFirst_level_service_name() == null || request.getFirst_level_service_name().length() < 1   ||
                request.getStep_number() == null                 )
        {
            response.setMessage("input is invalid");
            response.setStatus(100);
            return response;
        }
        if (request.getStep_number() == 1) {
            return doStep1(request);
        }
        if (request.getStep_number() == 2)
        {
            return doStep2(request);
        }



        response.setMessage("success");
        response.setStatus(0);
        return response;
    }
}
