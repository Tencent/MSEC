
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

import beans.request.IPPortPair;
import beans.request.ReleasePlan;
import beans.response.QueryReleasePlanDetailResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import org.codehaus.jackson.map.ObjectMapper;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Created by Administrator on 2016/2/2.
 * 查询某个发布计划的详细信息
 */
public class QueryReleasePlanDetail extends JsonRPCHandler {
    public QueryReleasePlanDetailResponse exec(ReleasePlan request)
    {
        QueryReleasePlanDetailResponse resp = new QueryReleasePlanDetailResponse();
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




        try {
            String sql = "select plan_id, first_level_service_name,second_level_service_name,"+
                    "config_tag,idl_tag, sharedobject_tag, status,memo,backend_task_status "+
                    "from t_release_plan where plan_id=?";
            List<Object> params = new ArrayList<Object>();
            params.add(request.getPlan_id());

            planList = util.findMoreRefResult(sql, params, ReleasePlan.class);
            if (planList.size() != 1)
            {
                resp.setStatus(100);
                resp.setMessage("Plan does NOT exist.");
                return resp;
            }
            resp.setDetail(planList.get(0));
            ReleasePlan detail = resp.getDetail();

            sql = "select dest_ip_list from t_release_plan where plan_id=?";
            params = new ArrayList<Object>();
            params.add(request.getPlan_id());

            Map<String, Object> res = util.findSimpleResult(sql,params);
            if (res.get("dest_ip_list") == null )
            {
                resp.setStatus(100);
                resp.setMessage("query dest ip list failed.");
                return resp;
            }
            String dest_ip_list = (String)(res.get("dest_ip_list"));
            ObjectMapper objectMapper = new ObjectMapper();
            Object o = objectMapper.readValue(dest_ip_list, new ArrayList<IPPortPair>().getClass());
            detail.setDest_ip_list((ArrayList<IPPortPair>)o);






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




        resp.setMessage("success");
        resp.setStatus(0);

        return resp;
    }
}
