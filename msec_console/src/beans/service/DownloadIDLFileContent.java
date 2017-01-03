
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

import beans.dbaccess.IDL;
import beans.dbaccess.SecondLevelServiceConfigTag;

import ngse.org.JsonRPCHandler;
import ngse.org.JsonRPCResponseBase;
import org.apache.log4j.Logger;

import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.nio.charset.Charset;

/**
 * Created by Administrator on 2016/1/28.
 * 下载某个版本的IDL 文件内容
 */
@WebServlet(name = "DownloadConfigFileContent")
public class DownloadIDLFileContent extends JsonRPCHandler {

    public JsonRPCResponseBase exec(IDL request) {
        Logger logger = Logger.getLogger(DownloadIDLFileContent.class);

        try {

            getHttpResponse().setCharacterEncoding("UTF-8");
            getHttpResponse().setContentType("application/text; charset=utf-8");
            PrintWriter out = getHttpResponse().getWriter();

            String first_level_service_name = request.getFirst_level_service_name();
            String second_level_service_name = request.getSecond_level_service_name();
            String tag_name = request.getTag_name();
            if (first_level_service_name == null || first_level_service_name.length() < 1 ||
                    second_level_service_name == null || second_level_service_name.length() < 1 ||
                    tag_name == null || tag_name.length() < 1) {
                out.printf("server:wrong parameters");
                return null;
            }
            String filename = IDL.getIDLFileName(first_level_service_name, second_level_service_name, tag_name);
            logger.error("download " + filename);


            File file = new File(filename);
            if (file.exists() && file.isFile()) {

                FileInputStream in = new FileInputStream(file);
                //DataInputStream dis = new DataInputStream(in);
                byte[] buf = new byte[1024];
                while (true) {
                    int len = in.read(buf);
                    if (len <= 0) {
                        in.close();
                        break;
                    }
                    out.write(new String(buf, 0, len, Charset.forName("utf8")));
                }

            } else {
                out.println("file not exist:" + filename);
                return null;
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return null;
        }
        return null;
    }



    /*
    protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

    }

    protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {

    }
    protected void service(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {

        Logger logger = Logger.getLogger(DownloadIDLFileContent.class);


        resp.setCharacterEncoding("UTF-8");
        resp.setContentType("application/text; charset=utf-8");
        PrintWriter out =  resp.getWriter();

        String first_level_service_name =  req.getParameter("first_level_service_name");
        String second_level_service_name =  req.getParameter("second_level_service_name");
        String tag_name = req.getParameter("tag_name");
        if (first_level_service_name == null || first_level_service_name.length()< 1||
                second_level_service_name == null ||second_level_service_name.length() < 1 ||
                tag_name == null ||tag_name.length() < 1){
            out.printf("server:wrong parameters");
            return;
        }
        String filename = IDL.getIDLFileName(first_level_service_name, second_level_service_name, tag_name);
        logger.error("download "+filename);


        File file = new File(filename);
        if(file.exists() && file.isFile()) {
            try {
                FileInputStream in = new FileInputStream(file);
                //DataInputStream dis = new DataInputStream(in);
                byte[] buf = new byte[1024];
                while (true) {
                    int len = in.read(buf);
                    if (len <= 0)
                    {
                        in.close();
                        break;
                    }
                    out.write(new String(buf, 0, len, Charset.forName("utf8")));
                }

            }
            catch (IOException e)
            {
                out.println("server:"+e.toString());
                return;
            }
        }
        else
        {
            out.println("file not exist:"+filename);
            return;
        }
    }
    */

}
