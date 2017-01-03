
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

import beans.request.AddServiceRequest;
import beans.response.AddServiceResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/27.
 * 新建一个异构服务，可以是一级服务，也可以是二级服务
 */
public class AddOddService extends JsonRPCHandler {
    public AddServiceResponse exec(AddServiceRequest request)
    {
        AddServiceResponse response = new AddServiceResponse();
        response.setMessage("unkown error.");
        response.setStatus(100);

        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getService_name() == null ||
                request.getService_name().equals("") ||
                request.getService_level() == null ||
                request.getService_level().equals(""))
        {
            response.setMessage("The name/level of service to be added should NOT be empty.");
            response.setStatus(100);
            return response;
        }
        if ( request.getService_level().equals("second_level") &&
                ( request.getService_parent() == null || request.getService_parent().equals("")))
        {
            response.setMessage("The first level service name  should NOT be empty.");
            response.setStatus(100);
            return response;
        }
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setMessage("DB connect failed.");
            response.setStatus(100);
            return response;
        }
        String sql;
        List<Object> params = new ArrayList<Object>();
        if (request.getService_level().equals("first_level"))
        {
             sql = "insert into t_first_level_service( first_level_service_name, type) values(?, 'odd')";

            params.add(request.getService_name());

        }
        else
        {
             sql = "insert into t_second_level_service(second_level_service_name, first_level_service_name, type) values(?,?, 'odd')";

            params.add(request.getService_name());
            params.add(request.getService_parent());
        }


        try {
            int addNum = util.updateByPreparedStatement(sql, params);
            if (addNum >= 0)
            {
                response.setAddNum(addNum);
                response.setMessage("success");
                response.setStatus(0);
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
        return  response;
    }
}
