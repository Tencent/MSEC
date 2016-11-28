
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
import msec.org.*;
import org.apache.log4j.Logger;

import javax.servlet.ServletContext;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * Created by Administrator on 2016/2/16.
 */
public class InstallServerProc implements Runnable {
    ArrayList<InstallPlan.IPInfo> infos;
    ServerInfo server;
    ClusterInfo cluster_info;
    String action;
    ServletContext servletContext;

    public  InstallServerProc(ArrayList<InstallPlan.IPInfo> infos_, ServerInfo server_, ClusterInfo cluster_info_, String action_, ServletContext context)
    {
        infos = infos_;
        server = server_;
        cluster_info = cluster_info_;
        action = action_;
        servletContext = context;
    }

    private void updateStatus(String status)
    {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "update t_install_plan set status=? where plan_id=?";
            List<Object> params = new ArrayList<Object>();
            params.add(status);
            params.add(cluster_info.getPlan_id());

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum != 1) {
                return;
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error(e.getMessage());
        } finally {
            util.releaseConn();
        }
    }

    private void updateStatus(String ip, int port, String status)
    {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "update t_install_plan set status=? where plan_id=? and ip=? and port=?";
            List<Object> params = new ArrayList<Object>();
            params.add(status);
            params.add(cluster_info.getPlan_id());
            params.add(ip);
            params.add(port);

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum != 1) {
                return;
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error(e.getMessage());
        } finally {
            util.releaseConn();
        }
    }

    @Override
    public void run() {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        RemoteShell remoteShell = new RemoteShell();
        String redisFile = servletContext.getRealPath("") + "/resources/redis.tgz";

        //先安装redis
        boolean return_ok = true;
        for(InstallPlan.IPInfo info : infos) {
            String result = remoteShell.SendFileToAgent(redisFile, "/tmp/redis.tgz", info.getIp());
            if (result == null || !result.equals("success")) {
                for (int port : info.getPort_status_map().keySet()) {
                    updateStatus(info.getIp(), port, "[ERROR] Remote shell fails to connect, please check the agent.");
                }
                logger.error(result);
                return;
            }
            for (int port : info.getPort_status_map().keySet()) {
                updateStatus(info.getIp(), port, "Redis package is sent.");
            }
            ArrayList<String> ip_port_list = new ArrayList<>();
            for (Map.Entry<Integer, String> entry : info.getPort_status_map().entrySet()) {
                String fmt =
                        "mkdir -p /data/redis\n" +
                                "cd /data/redis\n" +
                                "tar zxf /tmp/redis.tgz\n" +
                                "./sys.sh\n" +
                                "su -s /bin/bash -c './%s.sh %s %d %d' redis\n";
                String cmd = String.format(fmt, action, info.getIp(), entry.getKey(), info.getInstance_memory() * 1024 * 1024);
                StringBuffer output = new StringBuffer();
                remoteShell.SendCmdsToRunAndGetResultBack(cmd, info.getIp(), output);
                if (result == null || !result.equals("success")) {
                    return_ok = false;
                    logger.error(String.format("remote error:%s|%s", info.getIp(), result));
                    updateStatus(info.getIp(), entry.getKey(), "[ERROR] Remote shell fails to connect, please check the agent.");
                    break;
                } else {
                    logger.info(String.format("%s|%d|%s", info.getIp(), entry.getKey(), output));
                    if (!output.toString().equals("OK\n")) {
                        logger.error(String.format("start redis error:%s|%s", info.getIp(), output));
                        return_ok = false;
                        updateStatus(info.getIp(), entry.getKey(), "[ERROR] Redis fails to start, please check if the directory or the port is occupied.");
                    } else {
                        updateStatus(info.getIp(), entry.getKey(), "Redis starts.");
                    }
                }
            }
        }

        if(return_ok) {
            if(action.equals("create")) {
                JedisHelper helper = null;
                try {
                    if (server != null) {   //AddSet
                        helper = new JedisHelper(cluster_info.getPlan_id(), server.getIp(), server.getPort(), cluster_info.getCopy_num());
                        logger.info("AddSet");
                        logger.info(cluster_info.getIp_port_list());
                        helper.AddSet(cluster_info.getIp_port_list());
                    } else {//CreateSet
                        String[] ip_pair = cluster_info.getIp_port_list().get(0).split(":");
                        helper = new JedisHelper(cluster_info.getPlan_id(), ip_pair[0], Integer.parseInt(ip_pair[1]), cluster_info.getCopy_num());
                        helper.CreateSet(cluster_info.getIp_port_list());
                    }
                }
                catch (Exception e) {
                    updateStatus("[ERROR] Redis Cluster Exception!");
                    logger.error("Exception", e);
                } finally {
                    if(helper != null)
                        helper.close();
                }

            }
            else if(action.equals("recover")) {
                DBUtil util = new DBUtil();
                if (util.getConnection() == null) {
                    return;
                }
                JedisHelper helper = null;
                try {
                    //getOneOKServer
                    String sql = "select ip, port from t_service_info where first_level_service_name=? and second_level_service_name=? and set_id=? and group_id=? and status=? and master=1";
                    List<Object> params = new ArrayList<Object>();
                    params.add(server.getFirst_level_service_name());
                    params.add(server.getSecond_level_service_name());
                    params.add(server.getSet_id());
                    params.add(server.getGroup_id());
                    params.add("OK");
                    Map<String, Object> master_info = util.findSimpleResult(sql, params);
                    if (master_info.isEmpty()) {
                        updateStatus(server.getIp(), server.getPort(), "[ERROR] Replacing group doesn't have running master.");
                        return;
                    }
                    //Redis Operation
                    String master_ip = master_info.get("ip").toString();
                    int master_port = (Integer) master_info.get("port");
                    helper = new JedisHelper(cluster_info.getPlan_id(), master_ip, master_port, cluster_info.getCopy_num());
                    ArrayList<String> ips = new ArrayList<>();
                    ips.add(server.getIp() + ":" + server.getPort());
                    helper.RecoverSet(ips);
                } catch (Exception e) {
                    updateStatus(server.getIp(), server.getPort(), "[ERROR] Exception!");
                    logger.error("Exception", e);
                } finally {
                    util.releaseConn();
                    if(helper != null)
                        helper.close();
                }
            }
        }
    }
}
