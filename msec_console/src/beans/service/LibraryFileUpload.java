
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

import beans.dbaccess.LibraryFile;
import ngse.org.DBUtil;
import ngse.org.FileUploadServlet;
import org.apache.commons.fileupload.FileUpload;


import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.sql.SQLException;
import java.util.*;

/**
 * Created by Administrator on 2016/1/29.
 * 上传新版本库文件内容、tag、memo
 */
@WebServlet(name = "FileUpload")
public class LibraryFileUpload  extends FileUploadServlet{


    private String insertTable(String firstName, String secondName, String baseFileName, String memo)
    {
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            return "DB connect failed.";
        }
        String sql;
        List<Object> params = new ArrayList<Object>();

        sql = "insert into t_library_file(first_level_service_name,  second_level_service_name,file_name, memo) values(?,?,?,?)";
        params.add(firstName);
       params.add(secondName);
        params.add(baseFileName);
        params.add(memo);


        try {

            int addNum = util.updateByPreparedStatement(sql, params);

            if (addNum >= 0)
            {
             return "success";

            }
            return "insert failed";
        }
        catch (SQLException e)
        {
            return "insert failed";
        }
        finally {
            util.releaseConn();
        }


    }
    public String run(Map<String, String> fields, List<String>fileNames)  {



        String first_name = fields.get("first_level_service_name_of_new_library");
        String second_name = fields.get("second_level_service_name_of_new_library");
        if (first_name == null || first_name.length() < 1||
                second_name == null || second_name.length() < 1)
        {
            return ("{\"status\":100, \"message\":\"service name should NOT be empty.\"}");

        }
        if (fileNames.size() != 1)
        {
            return ("{\"status\":100, \"message\":\"file field missing.\"}");

        }
        File oldFile = new File(fileNames.get(0));
        String baseName = oldFile.getName();
        String destName = LibraryFile.getLibraryFileName(first_name, second_name, baseName);
        File newFile = new File(destName);
        if (!oldFile.renameTo(newFile))
        {
            return("{\"status\":100, \"message\":\"rename file failed.\"}");

        }
        //入库
        String memo = fields.get("new_library_memo");
        if (memo == null) {memo = "";}
        String result = insertTable(first_name, second_name, baseName, memo);
        if (result.equals("success"))
        {
            return String.format("{\"status\":0, \"message\":\"success\", \"file_name\":\"%s\"}", baseName);
        }
        else
        {
            return String.format("{\"status\":100, \"message\":\"%s\"}", result);
        }





    }

}
