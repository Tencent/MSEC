
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
import beans.response.AddIDLTagResponse;
import beans.response.AddSecondLevelServiceConfigTagResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/28.
 * 新增protocol buffer文件版本
 */
public class AddIDLTag extends JsonRPCHandler {
    private boolean SaveIDLContent(String filename, String content)
    {
        File file = new File(filename);
        file.delete();
        try {
            file.createNewFile();
            FileOutputStream out = new FileOutputStream(file);
            out.write(content.getBytes(Charset.forName("UTF-8")));
            out.close();
            return true;
        }catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }

    }
    public AddIDLTagResponse exec(IDL request)
    {
        AddIDLTagResponse response = new AddIDLTagResponse();
        response.setMessage("unkown error.");
        response.setStatus(100);
        //检查身份
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getTag_name() == null ||
                request.getTag_name().equals("") ||
                request.getFirst_level_service_name() == null || request.getFirst_level_service_name().length()<1||
                request.getSecond_level_service_name() == null || request.getSecond_level_service_name().length()<1||
                request.getContent() == null || request.getContent().length() < 1)

        {
            response.setMessage("Some request field is  empty.");
            response.setStatus(100);
            return response;
        }

        //连接数据库
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setMessage("DB connect failed.");
            response.setStatus(100);
            return response;
        }




        try {

            //先在服务器保存文件
            String filename = IDL.getIDLFileName(request.getFirst_level_service_name(),
                    request.getSecond_level_service_name(), request.getTag_name());
            if (!SaveIDLContent(filename, request.getContent()) )
            {
                response.setMessage("save file failed.");
                response.setStatus(100);
                return response;
            }

            //在数据库里记录一条
            String sql;
            List<Object> params = new ArrayList<Object>();

            sql = "insert into t_idl_tag(tag_name, memo, second_level_service_name,first_level_service_name) values(?,?,?,?)";
            params.add(request.getTag_name());
            if (request.getMemo() == null ){params.add("");}else{params.add(request.getMemo());}
            params.add(request.getSecond_level_service_name());
            params.add(request.getFirst_level_service_name());
            int addNum = util.updateByPreparedStatement(sql, params);

            if (addNum > 0)
            {
                response.setAddNumber(addNum);
                response.setMessage("success");
                response.setStatus(0);
                return response;
            }
            else
            {
                response.setAddNumber(addNum);
                response.setMessage("failed to insert table");
                response.setStatus(100);
                return response;
            }
        }
        catch (SQLException e)
        {
            response.setMessage("add record failed:"+e.toString());
            response.setStatus(100);
            e.printStackTrace();
            return response;
        }
        finally {
            util.releaseConn();
        }


    }
}
