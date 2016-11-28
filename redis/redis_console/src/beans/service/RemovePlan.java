
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

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/25.
 * 缩容流程
 */
public class RemovePlan extends JsonRPCHandler {

    public RemovePlanResponse exec(RemovePlanRequest request)
    {
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

        JedisHelper helper = null;
        List<ServerInfo> serverList;
        ServerInfo ok_server;

        try {
            String sql;
            List<Object> params = new ArrayList<Object>();
            //get plan_id;
            sql = "select copy_num, memory_per_instance, plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            try {
                ClusterInfo cluster_info = util.findSimpleRefResult(sql, params, ClusterInfo.class);
                if (!cluster_info.getPlan_id().isEmpty()) {
                    resp.setStatus(101);
                    resp.setMessage("Please wait until the ongoing plan finishes.");
                    return resp;
                }
            }catch (Exception e) {
                resp.setStatus(100);
                resp.setMessage("db query exception!");
                e.printStackTrace();
                return resp;
            }

            sql = "select ip, port, set_id, group_id, memory, master, status from t_service_info where first_level_service_name=? and second_level_service_name=? and master=1 and status='OK' limit 1";
            try {
                ok_server = util.findSimpleRefResult(sql, params, ServerInfo.class);
            } catch (Exception e) {
                resp.setStatus(100);
                resp.setMessage("OK server not found!");
                e.printStackTrace();
                return resp;
            }

            sql = "select ip, port, set_id, group_id, memory, master, 'Planning' as status from t_service_info where first_level_service_name=? and second_level_service_name=? and set_id=? order by group_id asc, master desc";
            params.add(request.getSet_id());
            try {
                serverList = util.findMoreRefResult(sql, params, ServerInfo.class);
            } catch (Exception e) {
                resp.setStatus(100);
                resp.setMessage("db query exception!");
                e.printStackTrace();
                return resp;
            }

            if(serverList == null || serverList.size() == 0) {
                resp.setStatus(100);
                resp.setMessage("set id error!");
                return resp;
            }

            String plan_id = AddSecondLevelServiceIPInfo.newPlanID();
            for (ServerInfo server : serverList) {
                sql = "insert into t_install_plan(plan_id, first_level_service_name, second_level_service_name, ip, port, set_id, group_id, memory, master, status, operation) values(?,?,?,?,?,?,?,?,?,?,?)";
                params.clear();
                params.add(plan_id);
                params.add(request.getFirst_level_service_name());
                params.add(request.getSecond_level_service_name());
                params.add(server.getIp());
                params.add(server.getPort());
                params.add(server.getSet_id());
                params.add(server.getGroup_id());
                params.add(server.getMemory());
                params.add(server.isMaster());
                params.add(server.getStatus());
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
                    e.printStackTrace();
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
                e.printStackTrace();
                return resp;
            }

            int copy_num = 0;
            ArrayList<String> ip_port_list = new ArrayList<>();
            for(ServerInfo server: serverList) {
                if(server.getSet_id().equals(serverList.get(0).getSet_id()) && server.getGroup_id() == serverList.get(0).getGroup_id())
                    copy_num++;
                if(server.isMaster())
                    ip_port_list.add(server.getIp()+":"+Integer.toString(server.getPort()));
            }

            try {
                helper = new JedisHelper(plan_id, ok_server.getIp(), ok_server.getPort(), copy_num);
                new Thread(new ClusterProc(helper, ip_port_list, "remove", getServlet().getServletContext())).start();
            } catch (Exception e) {
                resp.setStatus(101);
                resp.setMessage("Redis cluster exception!");
                e.printStackTrace();
                return resp;
            }

            resp.setMessage("success");
            resp.setPlan_id(plan_id);
            resp.setStatus(0);
            return resp;
        }
        finally {
            util.releaseConn();
            if(helper != null)
                helper.close();
        }
    }
}
