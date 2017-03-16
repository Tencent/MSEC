
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
 *
 */
public class InstallServerProc implements Runnable {
    ClusterInfo cluster_info;
    String data_dir;
    ServletContext servletContext;

    public  InstallServerProc(ClusterInfo cluster_info_, String data_dir_, ServletContext context)
    {
        cluster_info = cluster_info_;
        data_dir = data_dir_;
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
            logger.error(e);
        } finally {
            util.releaseConn();
        }
    }

    private void updateStatus(String ip, String status)
    {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "update t_install_plan set status=? where plan_id=? and ip=?";
            List<Object> params = new ArrayList<Object>();
            params.add(status);
            params.add(cluster_info.getPlan_id());
            params.add(ip);

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum != 1) {
                return;
            }
        } catch (Exception e) {
            logger.error(e);
        } finally {
            util.releaseConn();
        }
    }
    public static String strIPJoin(String[] ips, String sSep) {
        StringBuilder sbStr = new StringBuilder();
        for (int i = 0, il = ips.length; i < il; i++) {
            if (i > 0)
                sbStr.append(sSep);
            sbStr.append("\\\""+ips[i]+"\\\"");
        }
        return sbStr.toString();
    }
    @Override
    public void run() {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        RemoteShell remoteShell = new RemoteShell();
        String esFile = servletContext.getRealPath("") + "/resources/es.tgz";

        //先安装ES
        ArrayList<String> ok_ips = new ArrayList<>();

        String service_hosts = "";
        StringBuilder sbStr = new StringBuilder();
        String sep = " , ";
        if(cluster_info.getServer_list().size() > 0) {
            //add..
            boolean bSepAppend = false;
            for (ServerInfo info : cluster_info.getServer_list()) {
                if(!bSepAppend)
                {
                    bSepAppend = true;
                }
                else
                {
                    service_hosts += sep;
                }
                service_hosts += "\\\"" + info.getIp() + "\\\"";
            }
        }
        else {
            //create...
            service_hosts = strIPJoin(cluster_info.getReq_ips().toArray(new String[0]), sep);
        }



        for(String ip : cluster_info.getReq_ips()) {
            String result = remoteShell.SendFileToAgent(esFile, "/tmp/es.tgz", ip);
            if (result == null || !result.equals("success")) {
                logger.error(result);
                updateStatus(ip, "[ERROR] Remote shell fails to transfer the file, please check the agent.");
                return;
            }
            updateStatus(ip, "(2/5) ES package is sent.");

            String fmt =
                    "mkdir -p /data\n" +
                            "cd /data/\n" +
                            "tar zxf /tmp/es.tgz\n" +
                            "./sys.sh %s\n" +
                            "./create.sh %s %s %d %s \"%s\" %d\n";
            //__DATA_DIR__, __CLUSTER_NAME__, __IP__, __PORT__, __DATA_DIR__, __SERVICE_HOSTS__, __MINIMUM_ELECTS__
            String cmd = String.format(fmt, data_dir, cluster_info.getCluster_name(), ip, cluster_info.getReq_port(), data_dir, service_hosts, (cluster_info.getServer_list().size()+cluster_info.getReq_ips().size())/2+1);
            StringBuffer output = new StringBuffer();
            result = remoteShell.SendCmdsToRunAndGetResultBack(cmd, ip, output);
            if (result == null || !result.equals("success")) {
                logger.error(String.format("remote error:%s|%s", ip, result));
                updateStatus(ip, "[ERROR] Remote shell fails to connect, please check the agent.");
                break;
            } else {
                logger.info(String.format("%s|%s|%s", ip, output,cmd));
                if (!output.toString().startsWith("OK\n")) {
                    logger.error(String.format("start es error:%s|%s", ip, output));
                    updateStatus(ip, "[ERROR] ES fails to start, please check if the directory or the port is occupied.");
                } else {
                    updateStatus(ip, "(3/5) ES starts.");
                    ok_ips.add(ip);
                }
            }
        }

        if(ok_ips.size() > 0) {
            ESHelper helper = new ESHelper(cluster_info);
            logger.info("ClusterAdd|"+ok_ips.size());
            helper.ClusterAdd(ok_ips);
        }
    }
}
