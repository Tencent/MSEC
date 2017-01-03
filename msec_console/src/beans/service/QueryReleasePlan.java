
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

import beans.dbaccess.MachineInfo;
import beans.request.ReleasePlan;
import beans.response.QueryMachineListResponse;
import beans.response.QueryReleasePlanResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/2/2.
 * 查询发布计划的列表
 */
public class QueryReleasePlan extends JsonRPCHandler {
    public QueryReleasePlanResponse exec(ReleasePlan request)
    {
        QueryReleasePlanResponse resp = new QueryReleasePlanResponse();
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
        List<ReleasePlan> planList ;

        String sql = "select plan_id, first_level_service_name,second_level_service_name,"+
            "config_tag,idl_tag, sharedobject_tag, status,memo,backend_task_status from t_release_plan ";
        List<Object> params = new ArrayList<Object>();
        boolean hasWhere = false;
        if (request.getPlan_id() != null && request.getPlan_id().length() > 0)
        {
            sql += " where plan_id like ? ";
            params.add(request.getPlan_id());
            hasWhere = true;
        }
        String flsn = request.getFirst_level_service_name();
        String slsn = request.getSecond_level_service_name();
        if (flsn != null && flsn.length() > 0 &&
                slsn != null && slsn.length() > 0)
        {
            if (hasWhere)
            {
                sql = sql + " and first_level_service_name=? and second_level_service_name=? ";
            }
            else
            {
                sql = sql + " where first_level_service_name=? and second_level_service_name=? ";
            }
            hasWhere = true;
            params.add(flsn);
            params.add(slsn);
        }

        sql = sql + " order by plan_id desc";

        try {
            planList = util.findMoreRefResult(sql, params, ReleasePlan.class);

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



        resp.setPlan_list((ArrayList<ReleasePlan>) planList);
        resp.setMessage("success");
        resp.setStatus(0);

        return resp;
    }
}
