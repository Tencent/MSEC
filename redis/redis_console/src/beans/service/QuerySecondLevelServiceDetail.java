
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
 * Created by Administrator on 2016/1/25.
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
        List<ServerInfo> serverList ;
        ClusterInfo cluster_info;

        try {
            String sql = "select copy_num, memory_per_instance, plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
            List<Object> params = new ArrayList<Object>();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            cluster_info = util.findSimpleRefResult(sql, params, ClusterInfo.class);

            if(!cluster_info.getPlan_id().isEmpty()) {
                ArrayList<ServerInfo> planList;
                sql = "select ip, port, set_id, group_id, memory, master, status, operation, recover_host from t_install_plan where plan_id = ? order by group_id asc, master desc";
                params.clear();
                params.add(cluster_info.getPlan_id());
                planList = util.findMoreRefResult(sql, params, ServerInfo.class);

                String operation = "";
                int server_status = 0; //0:"ok", 1: "in progress" or 2: "error"
                for(ServerInfo server: planList ) {
                    operation = server.getOperation();
                    int status = 0;
                    if(!server.getStatus().startsWith("Done")) {
                        if(server.getStatus().startsWith("[ERROR]")) {
                            status = 2;
                        }
                        else
                            status = 1;
                    }
                    server_status = Math.max(status, server_status);
                }
                if(server_status != 1) {//not in progress
                    if(server_status == 0) {//ok, remove service_info first...TODO, needs to judge automatically
                        if(operation.equals("add")) {
                            for (ServerInfo server : planList) {
                                sql = "insert into t_service_info(first_level_service_name, second_level_service_name, ip, port, set_id, group_id, memory, master) values(?,?,?,?,?,?,?,?)";
                                params.clear();
                                params.add(request.getFirst_level_service_name());
                                params.add(request.getSecond_level_service_name());
                                params.add(server.getIp());
                                params.add(server.getPort());
                                params.add(server.getSet_id());
                                params.add(server.getGroup_id());
                                params.add(server.getMemory());
                                params.add(server.isMaster());
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
                                    e.printStackTrace();
                                    return resp;
                                }
                            }
                        }
                        else if(operation.equals("del")) {
                            sql = "delete from t_service_info where first_level_service_name=? and second_level_service_name=? and set_id=?";
                            params.clear();
                            params.add(request.getFirst_level_service_name());
                            params.add(request.getSecond_level_service_name());
                            params.add(planList.get(0).getSet_id());
                            int delNum = util.updateByPreparedStatement(sql, params);
                            if (delNum < 0) {
                                resp.setMessage("failed to delete table");
                                resp.setStatus(100);
                                return resp;
                            }
                        }
                        else if(operation.equals("rec")) {
                            String[] ip_pair = planList.get(0).getRecover_host().split(":");
                            sql = "update t_service_info set ip=?, port=? where first_level_service_name=? and second_level_service_name=? and ip=? and port=?";
                            params.clear();
                            params.add(planList.get(0).getIp());
                            params.add(planList.get(0).getPort());
                            params.add(request.getFirst_level_service_name());
                            params.add(request.getSecond_level_service_name());
                            params.add(ip_pair[0]);
                            params.add(Integer.parseInt(ip_pair[1]));
                            int addNum = util.updateByPreparedStatement(sql, params);
                            if (addNum < 0) {
                                resp.setMessage("failed to update table");
                                resp.setStatus(100);
                                return resp;
                            }
                        }
                    }

                    sql = "update t_second_level_service set plan_id=? where first_level_service_name=? and second_level_service_name=?";
                    params.clear();
                    params.add("");
                    params.add(request.getFirst_level_service_name());
                    params.add(request.getSecond_level_service_name());
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum < 0) {
                        resp.setMessage("failed to update table");
                        resp.setStatus(100);
                        return resp;
                    }
                    cluster_info.setPlan_id("");
                }
            }

            sql = "select ip, port, set_id, group_id, memory, master from t_service_info where first_level_service_name=? and second_level_service_name=? order by group_id asc, master desc";
            params.clear();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            serverList = util.findMoreRefResult(sql, params, ServerInfo.class);

            if(serverList.size() > 0  && cluster_info.getPlan_id().isEmpty()) { //working plan will cause config inconsistent...
                HashMap<String, ServerInfo> server_map = new HashMap<>();
                for (ServerInfo server : serverList) {
                    server_map.put(server.getIp() + ":" + server.getPort(), server);
                }
                JedisHelper helper = new JedisHelper(server_map, request.getFirst_level_service_name(), request.getSecond_level_service_name(), cluster_info);
                helper.CheckStatusDetail();
                if(!helper.isOK()) {
                    resp.setStatus(101);
                    resp.setMessage(helper.getError_message());
                    return resp;
                }
                if(helper.isChanged()) {
                    serverList = util.findMoreRefResult(sql, params, ServerInfo.class);
                }
            }
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

        resp.setServers((ArrayList<ServerInfo>)serverList);
        resp.setCluster_info(cluster_info);
        resp.setMessage("success");
        resp.setStatus(0);
        return resp;
    }
}
