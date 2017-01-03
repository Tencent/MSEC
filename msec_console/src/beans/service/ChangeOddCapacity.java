
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

import beans.request.ChangeCapacityRequest;
import beans.request.ChangeOddCapacityRequest;

import beans.request.IPPortPair;
import ngse.org.AccessZooKeeper;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.JsonRPCResponseBase;
 import org.apache.log4j.Logger;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/2/10.
 *  * 对异构服务进行扩缩容，将ip列表刷新到LB让其生效
 */
public class ChangeOddCapacity extends JsonRPCHandler {

    private String updateIPStatus(ChangeOddCapacityRequest request, boolean enable, DBUtil util)
    {

        List<Object> params = null;
        String sql = "";


        try {
            for (int i = 0; i < request.getIp_list().size(); i++) {

                if (enable) {
                    sql = " update t_second_level_service_ipinfo set status='enabled' " +
                            "where second_level_service_name=? and first_level_service_name=? and ip=? and port=?";
                } else {
                    sql = " update t_second_level_service_ipinfo set status='disabled' " +
                            "where second_level_service_name=? and first_level_service_name=? and ip=? and port=?";
                }
                params = new ArrayList<Object>();
                params.add(request.getSecond_level_service_name());
                params.add(request.getFirst_level_service_name());
                params.add(request.getIp_list().get(i).getIp());
                params.add(request.getIp_list().get(i).getPort());

                util.updateByPreparedStatement(sql, params);

            }
            return "success";
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }





    }
    public JsonRPCResponseBase exec(ChangeOddCapacityRequest request)
    {
        JsonRPCResponseBase response = new JsonRPCResponseBase();
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        Logger logger = Logger.getLogger(ChangeOddCapacity.class);

        //把ip列表刷新到LB的服务器里并下发给各个客户端
        AccessZooKeeper azk = new AccessZooKeeper();
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setStatus(100);
            response.setMessage("db connect failed");
            return response;
        }
        try
        {
            //step1：更新扩缩容的IP的状态
            if (request.getAction_type().equals("expand"))//扩容
            {
                updateIPStatus(request, true, util);
            }
            else // 缩容
            {
                updateIPStatus(request, false, util);

            }
            logger.info("update matchine's status OK, matchine Numbers:"+request.getIp_list().size());
            //step2: 获得该服务下所有enabled的IP，写入LB系统
            ArrayList<IPPortPair> ips = LoadBalance.getIPPortInfoByServiceName(request.getFirst_level_service_name(),
                    request.getSecond_level_service_name(),
                    util);
            if (ips == null)
            {
                response.setStatus(100);
                response.setMessage("get ip info from db failed");
                return response;
            }
            logger.info("get enabled IP, they will be write to LB server, number:"+ips.size());

            result = LoadBalance.writeOneServiceConfigInfo(azk, request.getFirst_level_service_name() + "/" + request.getSecond_level_service_name(),
                   false, ips);
            if (result == null || !result.equals("success"))
            {
                response.setStatus(100);
                response.setMessage(result);
                return response;
            }
            logger.info("write into LB server successfully.");


            response.setStatus(0);
            response.setMessage("success");
            return response;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            response.setStatus(100);
            response.setMessage(e.getMessage());
            return response;
        }
        finally {
            azk.disconnect();
            util.releaseConn();
        }
    }
}
