
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

import beans.request.DelStaffRequest;
import beans.response.DelStaffResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.JsonRPCResponseBase;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/26.
 * 删除用户
 */
public class DelStaff extends JsonRPCHandler {

    public DelStaffResponse exec(DelStaffRequest request)
    {
        DelStaffResponse response = new DelStaffResponse();
        response.setMessage("unkown error.");
        response.setStatus(100);
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getStaff_name() == null ||
                request.getStaff_name().equals(""))
        {
            response.setMessage("The name of staff to be deleted should NOT be empty.");
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
        String sql = "delete from t_staff where staff_name=?";
        List<Object> params = new ArrayList<Object>();
        params.add(request.getStaff_name());
        try {
            int delNum = util.updateByPreparedStatement(sql, params);
            if (delNum >= 0)
            {
                response.setMessage("success");
                response.setDeleteNumber(delNum);
                response.setStatus(0);
                return response;
            }
        }
        catch (SQLException e)
        {
            response.setMessage("Delete record failed:"+e.toString());
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
