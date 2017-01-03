
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

import beans.dbaccess.SecondLevelService;
import beans.request.AddSecondLevelServiceIPInfoRequest;
import beans.response.AddSecondLevelServiceIPInfoResponse;
import beans.response.AddServiceResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.Tools;
import org.apache.log4j.Logger;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/27.
 * 增加 标准服务的IP信息
 * 一开始没有规划好，搞的这几个bean的名字都怪怪的
 */
public class AddSecondLevelServiceIPInfo extends JsonRPCHandler {

    public AddSecondLevelServiceIPInfoResponse exec(AddSecondLevelServiceIPInfoRequest request)
    {
        AddSecondLevelServiceIPInfoResponse response = new AddSecondLevelServiceIPInfoResponse();
        response.setMessage("unkown error.");
        response.setStatus(100);
        Logger logger = Logger.getLogger(AddSecondLevelServiceIPInfo.class);

        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getIp() == null ||
                request.getIp().equals("") ||
                request.getPort() == null ||
                request.getStatus() == null ||
                request.getStatus().equals("") ||
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
            response.setMessage("IP format error!");
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
        /*
        if (request.getPort() == -1)// -1表示由该业务全局port配置项决定，在t_second_level_service表里
        {
            int port = getStandardServicePort(util, request.getFirst_level_service_name(),
                                                    request.getSecond_level_service_name());
            request.setPort(port);
        }
        logger.info("request.port="+request.getPort());
        */


        ArrayList<String> addedIPs = new ArrayList<String>();
        try {
            for (int i = 0; i < ips.size(); i++) {

                String sql;
                List<Object> params = new ArrayList<Object>();

                sql = "insert into t_second_level_service_ipinfo(ip,port,status, second_level_service_name,first_level_service_name, comm_proto) values(?,?,?,?,?,?)";
                params.add(ips.get(i));
                params.add(request.getPort());
                params.add(request.getStatus());
                params.add(request.getSecond_level_service_name());
                params.add(request.getFirst_level_service_name());
                params.add(request.getComm_proto());


                try {
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum >= 0) {

                        addedIPs.add(ips.get(i));
                        continue;
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
            response.setAddedIPs(addedIPs);
            response.setMessage("success");
            response.setStatus(0);
            return response;
        }
        finally {
            util.releaseConn();
        }




    }
}
