
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
import beans.request.InstallPlanRequest;
import beans.response.InstallPlanResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;

import java.sql.SQLException;
import java.util.*;

/**
 *
 * 扩容流程
 */
public class InstallPlan extends JsonRPCHandler {
    public InstallPlanResponse exec(InstallPlanRequest request) {
        Logger logger =  Logger.getLogger(InstallPlan.class);
        InstallPlanResponse resp = new InstallPlanResponse();

        String result = checkIdentity();
        if (!result.equals("success"))
        {
            resp.setStatus(99);
            resp.setMessage(result);
            return resp;
        }

        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            resp.setStatus(100);
            resp.setMessage("db connect failed!");
            return resp;
        }

        ServerInfo helper_server = null;

        try {
            if(request.getAdded_ips() == null || request.getAdded_ips().size() == 0) {
                resp.setStatus(101);
                resp.setMessage("Request field error.");
                return resp;
            }

            //get cluster info;
            ClusterInfo cluster_info = null;
            String sql = "select plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
            List<Object> params = new ArrayList<Object>();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            try {
                cluster_info = util.findSimpleRefResult(sql, params, ClusterInfo.class);
                if (cluster_info == null || !cluster_info.getPlan_id().isEmpty()) {
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

            ArrayList<ServerInfo> serviceList;
            ArrayList<String> ip_list = new ArrayList<>();
            int port = 0;

            //get current cluster ips
            sql = "select ip, port, status from t_service_info where first_level_service_name=? and second_level_service_name=?";
            params.clear();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());

            try {
                serviceList = util.findMoreRefResult(sql, params, ServerInfo.class);
            } catch (Exception e) {
                resp.setStatus(100);
                resp.setMessage("db query exception!");
                logger.error(e);
                return resp;
            }

            /*TODO
            if(serviceList.size() > 0) {
                for(ServerInfo info : serviceList) {
                    if( info.getStatus().equals("OK") ) {
                        helper_server = info;
                        break;
                    }
                }
            }
            */

            String plan_id = AddSecondLevelServiceIPInfo.newPlanID();
            for (String ip : request.getAdded_ips()) {
                sql = "insert into t_install_plan(plan_id, first_level_service_name, second_level_service_name, ip, port, status, operation) values(?,?,?,?,?,?,?)";
                params.clear();
                params.add(plan_id);
                params.add(request.getFirst_level_service_name());
                params.add(request.getSecond_level_service_name());
                params.add(ip);
                params.add(ESHelper.default_port);
                params.add("(1/5) Planning");
                params.add("add");
                try {
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum < 0) {
                        resp.setMessage("Failed to insert plan.");
                        resp.setStatus(100);
                        return resp;
                    }
                } catch (SQLException e) {
                    resp.setMessage("add record failed:" + e.toString());
                    resp.setStatus(100);
                    logger.error(e);
                    return resp;
                }

                ip_list.add(ip);
                port = ESHelper.default_port;
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
            cluster_info.setServer_list(serviceList);
            cluster_info.setReq_ips(ip_list);
            cluster_info.setReq_port(port);
            logger.info(serviceList);
            new Thread(new InstallServerProc( cluster_info, request.getData_dir(), getServlet().getServletContext())).start();

            resp.setPlan_id(plan_id);
            resp.setMessage("success");
            resp.setStatus(0);
            return resp;
        } finally {
            util.releaseConn();
        }
    }
}
