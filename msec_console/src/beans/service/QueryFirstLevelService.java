
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

import beans.dbaccess.FirstLevelService;
import beans.dbaccess.MachineInfo;
import beans.request.QueryFirstLevelServiceRequest;
import beans.response.QueryFirstLevelServiceResponse;
import beans.response.QueryMachineListResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import javax.servlet.http.Cookie;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/27.
 * 查一级标准服务
 */
public class QueryFirstLevelService extends JsonRPCHandler {


    public QueryFirstLevelServiceResponse exec(QueryFirstLevelServiceRequest request)
    {
        QueryFirstLevelServiceResponse resp = new QueryFirstLevelServiceResponse();

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
        List<FirstLevelService> serviceList ;


        String sql = "select first_level_service_name from t_first_level_service where type='standard' ";
        List<Object> params = new ArrayList<Object>();
        if (request.getService_name() != null && request.getService_name().length() > 0)
        {
            sql += " where first_level_service_name=? ";
            params.add(request.getService_name());
        }

        try {
            serviceList = util.findMoreRefResult(sql, params, FirstLevelService.class);

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



        resp.setService_list((ArrayList<FirstLevelService>) serviceList);
        resp.setMessage("success");
        resp.setStatus(0);

        return resp;
    }
}
