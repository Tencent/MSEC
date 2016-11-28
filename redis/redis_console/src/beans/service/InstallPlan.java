
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
 * Created by Administrator on 2016/1/25.
 * 扩容流程
 */
public class InstallPlan extends JsonRPCHandler {

    public static Comparator<ServerInfo> compByGroupId()
    {
        Comparator<ServerInfo> comp = new Comparator<ServerInfo>(){
            @Override
            public int compare(ServerInfo svr1, ServerInfo svr2)
            {
                if(svr1.getGroup_id() < svr2.getGroup_id())
                    return -1;
                else if(svr1.getGroup_id() > svr2.getGroup_id())
                    return 1;
                else {
                    if(svr1.isMaster())
                        return -1;
                    else
                        return 1;
                }
            }
        };
        return comp;
    }

    public class IPInfo {
        public IPInfo(String ip_, int instance_memory_) {
            ip = ip_;
            instance_memory = instance_memory_;
            port_status_map = new HashMap<>();
        }

        public HashMap<Integer, String> getPort_status_map() {
            return port_status_map;
        }

        public void setPort_status_map(HashMap<Integer, String> port_status_map) {
            this.port_status_map = port_status_map;
        }

        public String getIp() {
            return ip;
        }

        public void setIp(String ip) {
            this.ip = ip;
        }

        public int getInstance_memory() {
            return instance_memory;
        }

        public void setInstance_memory(int instance_memory) {
            this.instance_memory = instance_memory;
        }

        String ip;      //IP
        int instance_memory;    //memory per instance
        HashMap<Integer, String> port_status_map;       //端口 - 进度
    }

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
            if(request.getAdded_servers() == null || request.getAdded_servers().size() == 0) {
                resp.setStatus(101);
                resp.setMessage("Request field error.");
                return resp;
            }

            Collections.sort(request.getAdded_servers(), compByGroupId());

            //get cluster info;
            ClusterInfo cluster_info = null;
            String sql = "select copy_num, memory_per_instance, plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
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
                e.printStackTrace();
                return resp;
            }

            List<ServerInfo> serviceList;
            ArrayList<String> ip_port_list = new ArrayList<>();
            HashMap<String, IPInfo> ip_map = new HashMap<>();

            //get current redis cluster ips
            sql = "select ip, port, status from t_service_info where first_level_service_name=? and second_level_service_name=? order by group_id asc, master desc limit 1";
            params.clear();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());

            try {
                serviceList = util.findMoreRefResult(sql, params, ServerInfo.class);
            } catch (Exception e) {
                resp.setStatus(100);
                resp.setMessage("db query exception!");
                e.printStackTrace();
                return resp;
            }

            if(serviceList.size() > 0) {
                for(ServerInfo info : serviceList) {
                    if( info.getStatus().equals("OK") ) {
                        helper_server = info;
                        break;
                    }
                }
                if(helper_server == null) {
                    //没有可用机器
                    resp.setStatus(101);
                    resp.setMessage("Cluster state error!");
                    return resp;
                }
            }

            String plan_id = AddSecondLevelServiceIPInfo.newPlanID();
            for (ServerInfo server : request.getAdded_servers()) {
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
                params.add("Planning");
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
                    e.printStackTrace();
                    return resp;
                }

                ip_port_list.add(server.getIp()+":"+Integer.toString(server.getPort()));
                if(ip_map.containsKey(server.getIp())) {
                    ip_map.get(server.getIp()).getPort_status_map().put(server.getPort(), server.getStatus());
                }
                else {
                    IPInfo ipinfo = new IPInfo(server.getIp(), server.getMemory());
                    ipinfo.getPort_status_map().put(server.getPort(), server.getStatus());
                    ip_map.put(server.getIp(), ipinfo);
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

            cluster_info.setPlan_id(plan_id);
            cluster_info.setIp_port_list(ip_port_list);
            logger.info(ip_port_list);
            new Thread(new InstallServerProc(new ArrayList<IPInfo>(ip_map.values()), helper_server, cluster_info, "create", getServlet().getServletContext())).start();
/*
                try {
                    JedisHelper helper = null;
                    if (serviceList.size() > 0) {
                        helper = new JedisHelper(request.getPlan_id(), serviceList.get(0).getIp(), serviceList.get(0).getPort(), copy_num);
                        new Thread(new ClusterProc(helper, ip_port_list, "add", getServlet().getServletContext())).start();
                    } else {//CreateSet
                        helper = new JedisHelper(request.getPlan_id(), serverList.get(0).getIp(), serverList.get(0).getPort(), copy_num);
                        new Thread(new ClusterProc(helper, ip_port_list, "create", getServlet().getServletContext())).start();
                    }
                } catch (Exception e) {
                    resp.setStatus(101);
                    resp.setMessage("Redis cluster exception!");
                    e.printStackTrace();
                    return resp;
                }
*/

            resp.setPlan_id(plan_id);
            resp.setMessage("success");
            resp.setStatus(0);
            return resp;
        } finally {
            util.releaseConn();
        }
    }
}
