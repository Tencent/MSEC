
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

import beans.dbaccess.OddSecondLevelServiceIPInfo;
import beans.request.AddSecondLevelServiceIPInfoRequest;
import beans.request.IPPortPair;
import beans.response.AddOddSecondLevelServiceIPInfoResponse;
import beans.response.AddSecondLevelServiceIPInfoResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.Tools;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/27.
 * 新增 异构服务的IP信息
 */
public class AddOddSecondLevelServiceIPInfo extends JsonRPCHandler {
    public AddOddSecondLevelServiceIPInfoResponse exec(OddSecondLevelServiceIPInfo request)
    {
        AddOddSecondLevelServiceIPInfoResponse response = new AddOddSecondLevelServiceIPInfoResponse();
        response.setMessage("unkown error.");
        response.setStatus(100);

        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getIp() == null ||
                request.getIp().equals("") ||
                request.getSecond_level_service_name() == null||
                request.getSecond_level_service_name().equals(""))
        {
            response.setMessage("Some request field is  empty.");
            response.setStatus(100);
            return response;
        }
        ArrayList<String> ips = Tools.splitBySemicolon(request.getIp());
        if (ips == null || ips.size() == 0)
        {
            response.setMessage("IP should NOT be  empty.");
            response.setStatus(100);
            return response;
        }

        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setMessage("DB connect failed.");
            response.setStatus(100);
            return response;
        }
        ArrayList<IPPortPair> addedIP = new ArrayList<IPPortPair>();
        try {
            for (int i = 0; i < ips.size(); ++i) {
                String s = ips.get(i);
                int index  = s.indexOf(":");
                if (index <= 0 || index == (s.length()-1) )
                {
                    continue;
                }
                String ip = s.substring(0, index);
                int port= Integer.valueOf(s.substring(index+1));

                String sql;
                List<Object> params = new ArrayList<Object>();

                sql = "insert into t_second_level_service_ipinfo(ip,port,status, second_level_service_name,first_level_service_name, comm_proto) values(?,?,?,?,?,?)";
                params.add(ip);
                params.add(port);
                params.add("disabled");
                params.add(request.getSecond_level_service_name());
                params.add(request.getFirst_level_service_name());
                params.add(request.getComm_proto());


                try {
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum >= 0) {
                       IPPortPair pair = new IPPortPair();
                        pair.setIp(ip);
                        pair.setPort(port);
                        pair.setStatus("disabled");
                        addedIP.add(pair);

                    } else {
                        response.setMessage("failed to insert table");
                        response.setStatus(100);
                        return response;
                    }
                } catch (SQLException e) {
                    response.setMessage("add record failed:" + e.toString());
                    response.setStatus(100);
                    e.printStackTrace();
                    return response;
                }
            }
            response.setMessage("success");
            response.setStatus(0);
            response.setAddedIPs(addedIP);
            return response;
        }
        finally {
            util.releaseConn();
        }


    }
}
