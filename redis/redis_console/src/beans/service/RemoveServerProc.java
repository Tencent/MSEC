
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

import beans.dbaccess.ServerInfo;
import msec.org.DBUtil;
import org.apache.log4j.Logger;

import javax.servlet.ServletContext;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;

/**
 * Created by Administrator on 2016/2/16.
 */
public class RemoveServerProc implements Runnable {
    ServerInfo server;
    String plan_id;
    ServletContext servletContext;
    public RemoveServerProc(ServerInfo s, String p, ServletContext context)
    {
        servletContext = context;
        server = s;
        plan_id = p;
    }

    private void updateStatus(String status)
    {
        Logger logger = Logger.getLogger(RemoveServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "update t_install_plan set status=? where plan_id=? and ip=? and port=?";
            List<Object> params = new ArrayList<Object>();
            params.add(status);
            params.add(plan_id);
            params.add(server.getIp());
            params.add(server.getPort());

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum != 1) {
                return;
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error(e.getMessage());
            return;
        } finally {
            util.releaseConn();
        }
    }

    @Override
    public void run() {
        Logger logger = Logger.getLogger(RemoveServerProc.class);
        try {
            updateStatus("Step 0");
            TimeUnit.SECONDS.sleep(1);
            updateStatus("Step 1");
            TimeUnit.SECONDS.sleep(1);
            updateStatus("Step 2");
            TimeUnit.SECONDS.sleep(1);
            updateStatus("Step 3");
            TimeUnit.SECONDS.sleep(1);
            updateStatus("Done!");
        } catch (InterruptedException e) {
            e.printStackTrace();
            logger.error(e.getMessage());
        }
    }
}
