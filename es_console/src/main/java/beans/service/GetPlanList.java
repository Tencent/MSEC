
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

import beans.dbaccess.PlanInfo;
import beans.request.GetPlanListRequest;
import beans.response.GetPlanListResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

/**
 *
 * 查询任务列表
 */
public class GetPlanList extends JsonRPCHandler {

    final String status_ok = "任务完成";
    final String status_error = "任务异常中断";
    final String status_ip = "任务正在处理中";
    final String op_add = "扩容";
    final String op_remove = "缩容";
    final String op_recover = "死机恢复/机器替换";


    public GetPlanListResponse exec(GetPlanListRequest request)
    {
        Logger logger =  Logger.getLogger(GetPlanList.class);
        GetPlanListResponse resp = new GetPlanListResponse();

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

        List<PlanInfo> planList ;
        HashMap<String, String> plan_status_map = new HashMap<>();
        ArrayList<PlanInfo> plans = new ArrayList<>();

        try {
            String sql = "select plan_id, operation, status from t_install_plan where first_level_service_name=? and second_level_service_name=?" +
                    " and create_time>=? and create_time<=? group by plan_id, operation, status order by plan_id desc";
            List<Object> params = new ArrayList<Object>();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            params.add(request.getBegin_date() + " 00:00:00");
            params.add(request.getEnd_date() + " 23:59:59");
            planList = util.findMoreRefResult(sql, params, PlanInfo.class);

            for(PlanInfo plan : planList) {
                String status = plan_status_map.get(plan.getPlan_id());
                if(status == null) {
                    if(plan.getStatus().startsWith("Done")) {
                        plan_status_map.put(plan.getPlan_id(), status_ok);
                    }
                    else if(plan.getStatus().startsWith("[ERROR]")) {
                        plan_status_map.put(plan.getPlan_id(), status_error);
                    }
                    else {
                        plan_status_map.put(plan.getPlan_id(), status_ip);
                    }
                } else {
                    //ERROR > In Progress > Done.
                    if(!plan.getStatus().startsWith("Done")) {
                        if(!status.equals(status_error)) {
                            if(plan.getStatus().startsWith("[ERROR]")) {
                                plan_status_map.put(plan.getPlan_id(), status_error);
                            }
                            else {//in progress
                                if(status.equals(status_ok)) {
                                    plan_status_map.put(plan.getPlan_id(), status_ip);
                                }
                            }
                        }
                    }
                }
            }

            for(PlanInfo plan: planList) {
                String status = plan_status_map.remove(plan.getPlan_id());
                if(status != null) {
                    if(plan.getOperation().equals("add")) {
                        plan.setOperation(op_add);
                    }
                    else if(plan.getOperation().equals("del")) {
                        plan.setOperation(op_remove);
                    }
                    else {
                        plan.setOperation(op_recover);
                    }

                    plan.setStatus(status);
                    plans.add(plan);
                }
            }
        }
        catch (Exception e)
        {
            resp.setStatus(100);
            resp.setMessage("db query exception!");
            logger.error(e);
            return resp;
        }
        finally {
            util.releaseConn();
        }

        resp.setPlans(plans);
        resp.setMessage("success");
        resp.setStatus(0);
        return resp;
    }
}
