
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

import beans.dbaccess.CapacityBaseInfo;
import beans.dbaccess.CapacityDetailInfo;
import beans.dbaccess.SecondLevelService;
import beans.request.CapacityRequest;
import beans.request.IPPortPair;
import beans.response.QueryCapacityDetailResponse;
import beans.response.QueryCapacityListResponse;
import ngse.org.AccessZooKeeper;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.util.ArrayList;
import org.apache.log4j.Logger;

/**
 * Created by Administrator on 2016/3/2.
 */
public class QueryCapacityDetail extends JsonRPCHandler {

    //比对检验一下数据库里ip信息和lb里的ip信息是否一致，结果以可读的方式
    private String DiffDbAndLb( ArrayList<IPPortPair> iplist, AccessZooKeeper azk, String flsn, String slsn, DBUtil dbutil)
    {
        // todo:待完善


        return "DB and LB 一致";
    }

    public QueryCapacityDetailResponse exec(CapacityRequest request)
    {
        Logger logger = Logger.getLogger("QueryCapacityDetail");
                QueryCapacityDetailResponse resp = new QueryCapacityDetailResponse();
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
        ArrayList<CapacityDetailInfo> capacities = new ArrayList<CapacityDetailInfo>() ;

        AccessZooKeeper azk  = new AccessZooKeeper();


        try {



            //该服务的所有ip
            ArrayList<IPPortPair> iplist = LoadBalance.getIPPortInfoByServiceName(request.getFirst_level_service_name(),
                    request.getSecond_level_service_name(),  util);
            if (iplist == null) {
                resp.setMessage("failed to get ip list for " + request.getFirst_level_service_name() + "." + request.getSecond_level_service_name());
                resp.setStatus(100);
                return resp;
            }


            //获取每一个ip的负载信息

            int j;
            for (j = 0; j < iplist.size(); j++) {
                IPPortPair ip = iplist.get(j);
                String svcname = request.getFirst_level_service_name() + "/" + request.getSecond_level_service_name();
                try {
                    int load = LoadBalance.readOneServiceLoadInfo(azk, svcname, ip.getIp());
                    if (load >= 0) {
                        CapacityDetailInfo c = new CapacityDetailInfo();
                        c.setIp(ip.getIp());
                        c.setLoad_level(load);
                        capacities.add(c);
                    }
                }
                catch (Exception e)
                {
                    e.printStackTrace();
                    resp.setMessage(e.getMessage());
                    resp.setStatus(100);
                    return resp;
                }


            }


        }
        finally {
            util.releaseConn();
            azk.disconnect();
        }



        resp.setInfo(capacities);
        resp.setMessage("success");
        resp.setStatus(0);

        return resp;


    }
}
