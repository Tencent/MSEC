
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
import beans.request.RecoverPlanRequest;
import beans.response.RecoverPlanResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Created by Administrator on 2016/1/25.
 * 死机恢复/机器替换流程
 */
public class RecoverPlan extends JsonRPCHandler {

    public RecoverPlanResponse exec(RecoverPlanRequest request)
    {
        Logger logger = Logger.getLogger(RecoverPlan.class);
        RecoverPlanResponse resp = new RecoverPlanResponse();
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
        ServerInfo server = null;
        String plan_id = "";
        try {
            String[] ip_pair= request.getOld_host().split(":");

            if(ip_pair.length != 2) {
                resp.setStatus(101);
                resp.setMessage("Request host error!");
                return resp;
            }
            ServerInfo old_server;

            //get plan_id;
            String sql = "select copy_num, memory_per_instance, plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
            List<Object> params = new ArrayList<Object>();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            ClusterInfo cluster_info = util.findSimpleRefResult(sql, params, ClusterInfo.class);
            if(!cluster_info.getPlan_id().isEmpty()) {
                resp.setStatus(101);
                resp.setMessage("Please wait until the ongoing plan finishes.");
                return resp;
            }

            sql = "select ip, port, set_id, group_id, memory, master from t_service_info where first_level_service_name=? and second_level_service_name=? and ip=? and port=?";
            params.add(ip_pair[0]);
            params.add(Integer.parseInt(ip_pair[1]));

            old_server = util.findSimpleRefResult(sql, params, ServerInfo.class);
            if(old_server == null) {
                resp.setStatus(101);
                resp.setMessage("Recover plan error!");
                return resp;
            }

            sql = "select distinct set_id from t_service_info where first_level_service_name=? and second_level_service_name=? and ip=?";
            params.clear();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            params.add(request.getNew_ip());

            ArrayList<Map<String, Object>> set_infos = util.findModeResult(sql, params);
            if (set_infos.size() > 1 || (set_infos.size() == 1 && !set_infos.get(0).get("set_id").toString().equals(old_server.getSet_id()))) {
                resp.setStatus(101);
                resp.setMessage("Request IP already exists in other set!");
                return resp;
            }
            plan_id = AddSecondLevelServiceIPInfo.newPlanID();
            server = old_server;    //use the same settings except ip
            server.setFirst_level_service_name(request.getFirst_level_service_name());
            server.setSecond_level_service_name(request.getSecond_level_service_name());
            server.setIp(request.getNew_ip());
            server.setStatus("Planning");
            sql = "insert into t_install_plan(plan_id, first_level_service_name, second_level_service_name, ip, port, set_id, group_id, memory, master, status, operation, recover_host) values(?,?,?,?,?,?,?,?,?,?,?,?)";
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
            params.add("rec");
            params.add(request.getOld_host());
            int addNum = util.updateByPreparedStatement(sql, params);
            if (addNum < 0) {
                resp.setMessage("Failed to insert plan.");
                resp.setStatus(100);
                return resp;
            }

            sql = "update t_second_level_service set plan_id=? where first_level_service_name=? and second_level_service_name=?";
            params.clear();
            params.add(plan_id);
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            addNum = util.updateByPreparedStatement(sql, params);
            if (addNum < 0) {
                resp.setMessage("Failed to insert plan.");
                resp.setStatus(100);
                return resp;
            }

            InstallPlan.IPInfo info = new InstallPlan().new IPInfo(server.getIp(), server.getMemory());
            info.getPort_status_map().put(server.getPort(), server.getStatus());
            ArrayList<InstallPlan.IPInfo> infos = new ArrayList<>();
            infos.add(info);
            cluster_info.setPlan_id(plan_id);
            new Thread(new InstallServerProc(infos, server, cluster_info, "recover", getServlet().getServletContext())).start();
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

        resp.setPlan_id(plan_id);
        resp.setMessage("success");
        resp.setStatus(0);
        return resp;
    }
}
