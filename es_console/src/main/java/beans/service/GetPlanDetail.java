
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
import beans.request.GetPlanDetailRequest;
import beans.response.GetPlanDetailResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 *
 * 查询用户列表
 */
public class GetPlanDetail extends JsonRPCHandler {

    public GetPlanDetailResponse exec(GetPlanDetailRequest request)
    {
        Logger logger =  Logger.getLogger(GetPlanDetailResponse.class);
        GetPlanDetailResponse resp = new GetPlanDetailResponse();

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

        List<ServerInfo> serverList;
        int copy_num = 0;
        ArrayList<String> ip_port_list = new ArrayList<>();

        try {
            String sql;
            List<Object> params = new ArrayList<Object>();
            String plan_id = request.getPlan_id();

            if (request.getPlan_id() == null || request.getPlan_id().isEmpty()) {
                resp.setStatus(101);
                resp.setMessage("parameter error!");
                return resp;
            } else {
                sql = "select ip, port, status from t_install_plan where plan_id=? and first_level_service_name=? and second_level_service_name=?";
                params.clear();
                params.add(request.getPlan_id());
                params.add(request.getFirst_level_service_name());
                params.add(request.getSecond_level_service_name());
                try {
                    serverList = util.findMoreRefResult(sql, params, ServerInfo.class);
                } catch (Exception e) {
                    resp.setStatus(100);
                    resp.setMessage("db query exception!");
                    logger.error(e);
                    return resp;
                }
            }

            if(serverList.size() == 0) {
                resp.setStatus(100);
                resp.setMessage("plan id error!");
                return resp;
            }


            boolean finished = true;
            for (ServerInfo server : serverList) {
                if (!server.getStatus().startsWith("Done")) {
                    if (server.getStatus().startsWith("[ERROR]")) {
                        finished = true;
                        break;
                    } else{ //in progress...
                        finished = false;
                    }
                }
            }

            resp.setFinished(finished);
            resp.setServers((ArrayList<ServerInfo>) serverList);
            resp.setMessage("success");
            resp.setStatus(0);
            return resp;
        }
        finally {
            util.releaseConn();
        }
    }
}
