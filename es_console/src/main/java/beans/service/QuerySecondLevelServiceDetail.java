
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
import beans.request.QuerySecondLevelServiceDetailRequest;
import beans.response.QuerySecondLevelServiceDetailResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

/**
 *
 * 查询列表
 */
public class QuerySecondLevelServiceDetail extends JsonRPCHandler {

    public QuerySecondLevelServiceDetailResponse exec(QuerySecondLevelServiceDetailRequest request)
    {
        Logger logger = Logger.getLogger(QuerySecondLevelServiceDetail.class);
        QuerySecondLevelServiceDetailResponse resp = new QuerySecondLevelServiceDetailResponse();

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
        List<ServerInfo> serverList = new ArrayList<>();
        ClusterInfo cluster_info;

        try {
            String sql = "select plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
            List<Object> params = new ArrayList<Object>();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            cluster_info = util.findSimpleRefResult(sql, params, ClusterInfo.class);

            if (!cluster_info.getPlan_id().isEmpty()) {
                ArrayList<ServerInfo> planList;
                sql = "select ip, port, status, operation from t_install_plan where plan_id = ?";
                params.clear();
                params.add(cluster_info.getPlan_id());
                planList = util.findMoreRefResult(sql, params, ServerInfo.class);

                String operation = "";
                int server_status = 0; //0:"ok", 1: "error" or 2: "in progress"
                for (ServerInfo server : planList) {
                    operation = server.getOperation();
                    int status = 0;
                    if (!server.getStatus().startsWith("Done")) {
                        if (server.getStatus().startsWith("[ERROR]")) {
                            status = 1;
                        } else
                            status = 2;
                    }
                    server_status = Math.max(status, server_status);
                }


                if(server_status != 2) {//not in progress
                    if (operation.equals("add")) {
                        for (ServerInfo server : planList) {
                            if(server.getStatus().startsWith("Done")) {
                                sql = "insert into t_service_info(first_level_service_name, second_level_service_name, ip, port) values(?,?,?,?)";
                                params.clear();
                                params.add(request.getFirst_level_service_name());
                                params.add(request.getSecond_level_service_name());
                                params.add(server.getIp());
                                params.add(server.getPort());
                                try {
                                    int addNum = util.updateByPreparedStatement(sql, params);
                                    if (addNum < 0) {
                                        resp.setMessage("Database fails to insert.");
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
                        }
                    }
                    /*
                    else if(operation.equals("del")) {
                        //nothing to do
                    }
                    */

                    sql = "update t_second_level_service set plan_id=? where first_level_service_name=? and second_level_service_name=?";
                    params.clear();
                    params.add("");
                    params.add(request.getFirst_level_service_name());
                    params.add(request.getSecond_level_service_name());
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum < 0) {
                        resp.setMessage("Database fails to update.");
                        resp.setStatus(100);
                        return resp;
                    }
                    cluster_info.setPlan_id("");
                }
            }

            sql = "select ip, port from t_service_info where first_level_service_name=? and second_level_service_name=? order by id asc";
            params.clear();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            serverList = util.findMoreRefResult(sql, params, ServerInfo.class);
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
        resp.setServers((ArrayList<ServerInfo>)serverList);
        resp.setCluster_info(cluster_info);
        resp.setMessage("success");
        resp.setStatus(0);
        return resp;
    }
}
