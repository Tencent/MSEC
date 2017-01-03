
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

import ngse.org.ServletConfig;
import org.apache.commons.fileupload.FileItem;
import org.apache.commons.fileupload.FileUploadException;
import org.apache.commons.fileupload.disk.DiskFileItemFactory;
import org.apache.commons.fileupload.servlet.ServletFileUpload;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.File;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Created by Administrator on 2016/1/29.
 */
public class FileUploadTool {
    static public String FileUpload( Map<String, String> fields, List<String> filesOnServer,
                                     HttpServletRequest request, HttpServletResponse response)
    {

        boolean isMultipart = ServletFileUpload.isMultipartContent(request);
        
        DiskFileItemFactory factory = new DiskFileItemFactory();
        int MaxMemorySize = 10000000;
        int MaxRequestSize = MaxMemorySize;
        String tmpDir = System.getProperty("TMP", "/tmp");
        System.out.printf("temporary directory:%s", tmpDir);


        factory.setSizeThreshold(MaxMemorySize);
        factory.setRepository(new File(tmpDir));

// Create a new file upload handler
        ServletFileUpload upload = new ServletFileUpload(factory);

// Set overall request size constraint
        upload.setSizeMax(MaxRequestSize);

// Parse the request
        try {
            List<FileItem> items = upload.parseRequest(request);
            // Process the uploaded items
            Iterator<FileItem> iter = items.iterator();
            while (iter.hasNext()) {
                FileItem item = iter.next();
                if (item.isFormField()) {//普通的k -v字段
                    String name = item.getFieldName();
                    String value = item.getString();
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
                    System.out.printf("upload file:%s", localFileName);
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
            return e.toString();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.toString();
        }

    }
}
