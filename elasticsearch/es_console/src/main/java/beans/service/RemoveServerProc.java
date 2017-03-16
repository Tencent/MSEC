
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
import msec.org.DBUtil;
import msec.org.RemoteShell;
import org.apache.log4j.Logger;

import javax.servlet.ServletContext;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 *
 */
public class RemoveServerProc implements Runnable {
    ClusterInfo cluster_info;
    ServletContext servletContext;
    public RemoveServerProc(ClusterInfo cluster_info_, ServletContext context)
    {
        servletContext = context;
        cluster_info = cluster_info_;
    }

    private void updateStatus(String ip, String status)
    {
        Logger logger = Logger.getLogger(RemoveServerProc.class);
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
            return;
        } finally {
            util.releaseConn();
        }
    }
    private void deleteServer(String ip)
    {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "delete from t_service_info where first_level_service_name=? and second_level_service_name=? and ip=?";
            List<Object> params = new ArrayList<Object>();
            params.add(cluster_info.getFirst_level_service_name());
            params.add(cluster_info.getSecond_level_service_name());
            params.add(ip);

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum < 0) {
                logger.error(String.format("delete_server ERROR|%s", ip));
                return;
            }
        } catch (Exception e) {
            logger.error(e);
            return;
        } finally {
            util.releaseConn();
        }
    }

    @Override
    public void run() {
        Logger logger = Logger.getLogger(RemoveServerProc.class);
        RemoteShell remoteShell = new RemoteShell();
        for(String ip : cluster_info.getReq_ips()) {
            String cmd = "/data/stop.sh | grep -v ok\n";
            StringBuffer output = new StringBuffer();
            String result = remoteShell.SendCmdsToRunAndGetResultBack(cmd, ip, output);
            if (result == null || !result.equals("success")) {
                logger.error(String.format("remote error:%s|%s", ip, result));
                updateStatus(ip, "[ERROR] Remote shell fails to connect, please check the agent.");
                break;
            }
            else {
                logger.info(String.format("%s|%s|%s", ip, output, cmd));
                if (!output.toString().isEmpty()) {
                    logger.error(String.format("remove es node error:%s|%s", ip, output));
                    updateStatus(ip, "[ERROR] ES node fails to stop.");
                } else {
                    updateStatus(ip, "Done.");
                    deleteServer(ip);
                }
            }
        }
    }
}
