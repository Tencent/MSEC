
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

import beans.request.AddNewMachineRequest;

import beans.response.AddNewStaffResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.JsonRPCResponseBase;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/26.
 * 新增开发编译机
 */
public class AddNewMachine extends JsonRPCHandler {
    public JsonRPCResponseBase exec(AddNewMachineRequest request)
    {
        JsonRPCResponseBase response = new JsonRPCResponseBase();
        response.setMessage("unkown error.");
        response.setStatus(100);

        //检查用户身份
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getMachine_name() == null ||
                request.getMachine_name().equals("") ||
                request.getMachine_ip() == null ||
                request.getMachine_ip().equals(""))
        {
            response.setMessage("The name/ip of machine to be added should NOT be empty.");
            response.setStatus(100);
            return response;
        }
        //连接并插入数据库
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setMessage("DB connect failed.");
            response.setStatus(100);
            return response;
        }
        String sql = "insert into t_machine(machine_name, machine_ip,os_version, gcc_version, java_version) values(?,?,?,?,?)";
        List<Object> params = new ArrayList<Object>();
        params.add(request.getMachine_name());
        params.add(request.getMachine_ip());
        params.add(request.getOs_version());
        params.add(request.getGcc_version());
        params.add(request.getJava_version());

        try {
            int addNum = util.updateByPreparedStatement(sql, params);
            if (addNum >= 0)
            {
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
