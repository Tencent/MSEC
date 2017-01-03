
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

import beans.service.LibraryFileUpload;
import beans.service.SharedobjectUpload;
import org.apache.commons.fileupload.FileItem;
import org.apache.commons.fileupload.FileUploadException;
import org.apache.commons.fileupload.disk.DiskFileItemFactory;
import org.apache.commons.fileupload.servlet.ServletFileUpload;

import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.*;

/**
 * Created by Administrator on 2016/2/22.
 */
@WebServlet(name = "FileUploadServlet")
public class FileUploadServlet extends HttpServlet {



    //处理multi-part格式的http请求
    //将key-value字段放到fields里返回
    //将文件保存到tmp目录，并将文件名保存到filesOnServer列表里返回
    static protected String FileUpload( Map<String, String> fields, List<String> filesOnServer,
                                     HttpServletRequest request, HttpServletResponse response)
    {

        boolean isMultipart = ServletFileUpload.isMultipartContent(request);
        // Create a factory for disk-based file items
        DiskFileItemFactory factory = new DiskFileItemFactory();
        int MaxMemorySize = 200000000;
        int MaxRequestSize = MaxMemorySize;
        String tmpDir = System.getProperty("TMP", "/tmp");

        factory.setSizeThreshold(MaxMemorySize);
        factory.setRepository(new File(tmpDir));
        ServletFileUpload upload = new ServletFileUpload(factory);
        upload.setHeaderEncoding("utf8");
        upload.setSizeMax(MaxRequestSize);
        try {
            List<FileItem> items = upload.parseRequest(request);
            // Process the uploaded items
            Iterator<FileItem> iter = items.iterator();
            while (iter.hasNext()) {
                FileItem item = iter.next();
                if (item.isFormField()) {//普通的k -v字段

                    String name = item.getFieldName();
                    String value = item.getString("utf-8");
                    fields.put(name, value);
                }
                else {

                    String fieldName = item.getFieldName();
                    String fileName = item.getName();
                    if (fileName == null || fileName.length() < 1)
                    {
                        return "file name is empty.";
                    }
                    String localFileName = ServletConfig.fileServerRootDir+File.separator+"tmp"+File.separator+fileName;
                    String contentType = item.getContentType();
                    boolean isInMemory = item.isInMemory();
                    long sizeInBytes = item.getSize();
                    File uploadedFile = new File(localFileName);
                    item.write(uploadedFile);
                    filesOnServer.add(localFileName);
                }


            }
            return "success";
        }catch (FileUploadException e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }

    }
    public String run(Map<String, String> fields,List<String> fileNames)
    {
        return "{\"status\":100, \"message\":\"default run method is called\"}";
    }

    protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {


        Map<String, String> fields = new HashMap<String, String>();
        List<String> fileNames = new ArrayList<String>();

        request.setCharacterEncoding("UTF-8");

        String result = FileUpload(fields, fileNames, request, response);

        response.setCharacterEncoding("UTF-8");
        response.setContentType("text/html; charset=utf-8");
        PrintWriter out =  response.getWriter();

        if (result == null || !result.equals("success"))
        {
            out.printf("{\"status\":100, \"message\":\"%s\"}", result == null? "":result);
            return;
        }
        String handleClass = fields.get("handleClass");
        if (handleClass == null || handleClass.length() < 1)
        {
            out.write("{\"status\":100, \"message\":\"unkown handle class\"}");
            return;
        }
        try {
            //加载handleClass字段指定的类，并创建它的实例
            Class<?> clazz = Class.forName(handleClass);
            FileUploadServlet servlet = (FileUploadServlet) clazz.newInstance();
            out.write(servlet.run(fields, fileNames));

        }
        catch (Exception e)
        {
            e.printStackTrace();
            out.printf("{\"status\":100, \"message\":\"%s\"}", e.getMessage());
            return;
        }
        /*
        if (handleClass != null && handleClass.equals("beans.service.LibraryFileUpload"))
        {
            out.write(new LibraryFileUpload().run(fields, fileNames));
            return;
        }
        if (handleClass != null && handleClass.equals("beans.service.SharedobjectUpload"))
        {
            out.write(new SharedobjectUpload().run(fields, fileNames));
            return;

        }
        */




    }


}
