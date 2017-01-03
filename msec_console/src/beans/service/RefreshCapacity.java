
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

import beans.request.CapacityRequest;
import beans.request.ChangeCapacityRequest;
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
 * 对标准服务进行扩缩容，将ip列表刷新到LB让其生效
 */
public class RefreshCapacity extends JsonRPCHandler {



    public JsonRPCResponseBase exec(CapacityRequest request)
    {
        JsonRPCResponseBase response = new JsonRPCResponseBase();
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        Logger logger = Logger.getLogger(RefreshCapacity.class);

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
                   true, ips);
            if (result == null || !result.equals("success"))
            {
                response.setStatus(100);
                response.setMessage(result);
                return response;
            }
            logger.info("write into LB server successfully.");
            //step3: 更新涉及到的IP为key的配置信息
            ArrayList<String> whiteNameList = new ArrayList<>();
            for (int i = 0; i < ips.size(); i++) {
                whiteNameList.add(ips.get(i).getIp());
            }

            logger.info("update the IP-key information in LB server");
            result = LoadBalance.writeIPConfigInfo(azk, whiteNameList);
            if (result == null || !result.equals("success"))
            {
                response.setStatus(100);
                response.setMessage(result);
                return response;
            }

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
