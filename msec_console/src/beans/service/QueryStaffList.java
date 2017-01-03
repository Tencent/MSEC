
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

import beans.dbaccess.StaffInfo;
import beans.request.QueryStaffListRequest;
import beans.response.QueryStaffListResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/25.
 * 查询用户列表
 */
public class QueryStaffList extends JsonRPCHandler {

    public QueryStaffListResponse exec(QueryStaffListRequest request)
    {
        QueryStaffListResponse resp = new QueryStaffListResponse();
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            resp.setStatus(99);
            resp.setMessage(result);
            return resp;
        }
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            resp.setStatus(100);
            resp.setMessage("db connect failed!");
            return resp;
        }
        List<StaffInfo> staffInfoList ;
        //System.out.printf("name:%s, phone:%s\n", request.getStaff_name(), request.getStaff_phone());

        String sql = "select staff_name, staff_phone from t_staff ";
        List<Object> params = new ArrayList<Object>();
        if (request.getStaff_name() != null && request.getStaff_name().length() > 0)
        {
            sql += " where staff_name=? ";
            params.add(request.getStaff_name());
        }
        else if (request.getStaff_phone() != null && request.getStaff_phone().length() > 0)
        {
            sql += " where staff_phone=? ";
            params.add(request.getStaff_phone());
        }
        try {
            staffInfoList = util.findMoreRefResult(sql, params, StaffInfo.class);

        }
        catch (Exception e)
        {
            resp.setStatus(100);
            resp.setMessage("db query exception!");
            e.printStackTrace();
            return resp;
        }
        finally {
            util.releaseConn();
        }



        resp.setStaff_list((ArrayList<StaffInfo>)staffInfoList);
        resp.setMessage("success");
        resp.setStatus(0);

        return resp;


    }
}
