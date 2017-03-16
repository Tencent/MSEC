
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


package msec.org;

import org.apache.commons.fileupload.FileItem;
import org.apache.commons.fileupload.FileUploadException;
import org.apache.commons.fileupload.disk.DiskFileItemFactory;
import org.apache.commons.fileupload.servlet.ServletFileUpload;
import org.apache.log4j.Logger;

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
 *
 */
@WebServlet(name = "FileUploadServlet")
public class FileUploadServlet extends HttpServlet {

    //处理multi-part格式的http请求
    //将key-value字段放到fields里返回
    //将文件保存到tmp目录，并将文件名保存到filesOnServer列表里返回
    static protected String FileUpload( Map<String, String> fields, List<String> filesOnServer,
                                     HttpServletRequest request, HttpServletResponse response)
    {
        Logger logger = Logger.getLogger(FileUploadServlet.class);
        boolean isMultipart = ServletFileUpload.isMultipartContent(request);
        // Create a factory for disk-based file items
        DiskFileItemFactory factory = new DiskFileItemFactory();
        int MaxMemorySize = 10000000;
        int MaxRequestSize = MaxMemorySize;
        String tmpDir = System.getProperty("TMP", "/tmp");
        //System.out.printf("temporary directory:%s", tmpDir);

// Set factory constraints
        factory.setSizeThreshold(MaxMemorySize);
        factory.setRepository(new File(tmpDir));

// Create a new file upload handler
        ServletFileUpload upload = new ServletFileUpload(factory);
        upload.setHeaderEncoding("utf8");


// Set overall request size constraint
        upload.setSizeMax(MaxRequestSize);

// Parse the request
        try {
            @SuppressWarnings("unchecked")
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
                    //System.out.printf("upload file:%s", localFileName);
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
            logger.error(e);
            return e.toString();
        }
        catch (Exception e)
        {
            logger.error(e);
            return e.toString();
        }

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
        if (handleClass != null && handleClass.equals("beans.service.LibraryFileUpload"))
        {
            //out.write(new LibraryFileUpload().run(fields, fileNames));
            return;
        }
        if (handleClass != null && handleClass.equals("beans.service.SharedobjectUpload"))
        {
          //  out.write(new SharedobjectUpload().run(fields, fileNames));
            return;

        }
        out.write("{\"status\":100, \"message\":\"unkown handle class\"}");
        return;


    }


}
