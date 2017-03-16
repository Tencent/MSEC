
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

import beans.dbaccess.ClusterInfo;
import beans.dbaccess.ServerInfo;
import beans.request.RemovePlanRequest;
import beans.response.RemovePlanResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 *
 * 缩容流程
 */
public class RemovePlan extends JsonRPCHandler {

    public RemovePlanResponse exec(RemovePlanRequest request)
    {
        Logger logger = Logger.getLogger(RemovePlanResponse.class);
        RemovePlanResponse resp = new RemovePlanResponse();
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

        ESHelper helper = null;
        ClusterInfo cluster_info;

        try {
            String sql;
            List<Object> params = new ArrayList<Object>();
            //get plan_id;
            sql = "select plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            try {
                cluster_info = util.findSimpleRefResult(sql, params, ClusterInfo.class);
                if (!cluster_info.getPlan_id().isEmpty()) {
                    resp.setStatus(101);
                    resp.setMessage("Please wait until the ongoing plan finishes.");
                    return resp;
                }
            }catch (Exception e) {
                resp.setStatus(100);
                resp.setMessage("db query exception!");
                logger.error(e);
                return resp;
            }

            String plan_id = AddSecondLevelServiceIPInfo.newPlanID();
            int port = ESHelper.default_port;
            for (String ip : request.getRemove_ips()) {
                sql = "insert into t_install_plan(plan_id, first_level_service_name, second_level_service_name, ip, port, status, operation) values(?,?,?,?,?,?,?)";
                params.clear();
                params.add(plan_id);
                params.add(request.getFirst_level_service_name());
                params.add(request.getSecond_level_service_name());
                params.add(ip);
                params.add(port);
                params.add("(1/2) Planning");
                params.add("del");

                try {
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum < 0) {
                        resp.setMessage("failed to insert table");
                        resp.setStatus(100);
                        return resp;
                    }
                } catch (SQLException e) {
                    resp.setMessage("add record failed:" + e.toString());
                    resp.setStatus(100);
                    logger.error(e);
                    return resp;
                }
            }

            sql = "update t_second_level_service set plan_id=? where first_level_service_name=? and second_level_service_name=?";
            params.clear();
            params.add(plan_id);
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            try {
                int addNum = util.updateByPreparedStatement(sql, params);
                if (addNum < 0) {
                    resp.setMessage("Failed to insert plan.");
                    resp.setStatus(100);
                    return resp;
                }
            }
            catch (SQLException e)
            {
                resp.setStatus(100);
                resp.setMessage("db query exception!");
                logger.error(e);
                return resp;
            }

            cluster_info.setFirst_level_service_name(request.getFirst_level_service_name());
            cluster_info.setSecond_level_service_name(request.getSecond_level_service_name());
            cluster_info.setPlan_id(plan_id);
            cluster_info.setReq_ips(request.getRemove_ips());
            cluster_info.setReq_port(port);
            new Thread(new RemoveServerProc( cluster_info, getServlet().getServletContext())).start();

            resp.setMessage("success");
            resp.setPlan_id(plan_id);
            resp.setStatus(0);
            return resp;
        }
        finally {
            util.releaseConn();
        }
    }
}
