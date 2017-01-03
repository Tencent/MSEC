
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
import beans.dbaccess.SecondLevelService;
import beans.request.CapacityRequest;
import beans.request.IPPortPair;
import beans.response.QueryCapacityListResponse;
import ngse.org.AccessZooKeeper;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.util.ArrayList;
import java.util.List;
import org.apache.log4j.Logger;

/**
 * Created by Administrator on 2016/2/9.
 * 查询业务的容量信息
 */
public class QueryCapacityList extends JsonRPCHandler {


    public QueryCapacityListResponse exec(CapacityRequest request)
    {
        Logger logger = Logger.getLogger("QueryCapacityList");
        QueryCapacityListResponse resp = new QueryCapacityListResponse();
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
        List<CapacityBaseInfo> capacities = new ArrayList<CapacityBaseInfo>() ;

        AccessZooKeeper azk  = new AccessZooKeeper();



        try {
            if (request.getFirst_level_service_name() == null || request.getFirst_level_service_name().length() < 1 ||
                    request.getSecond_level_service_name() == null || request.getSecond_level_service_name().length() < 1)
            {
                //所有服务
                ArrayList<SecondLevelService>  svcList = LoadBalance.getAllService(util);
                if (svcList == null)
                {
                    resp.setMessage("failed to get all service list");
                    resp.setStatus(100);
                    return resp;
                }
                for (int i = 0; i < svcList.size(); i++) { //每一个服务
                    SecondLevelService svc = svcList.get(i);

                    if (!svc.getType().equals("standard"))
                    {
                        continue;
                    }

                    //用户可能指定了一级服务名进行过滤，即只查看该一级服务下所有二级服务的情况
                    if (request.getFirst_level_service_name() != null &&
                            request.getFirst_level_service_name().length() > 0)
                    {
                        if (!svc.getFirst_level_service_name().equals(request.getFirst_level_service_name()))
                        {
                            continue;
                        }
                    }


                    CapacityBaseInfo c = new CapacityBaseInfo();
                    //该服务的所有ip
                    ArrayList<IPPortPair> iplist = LoadBalance.getIPPortInfoByServiceName(svc.getFirst_level_service_name(),
                            svc.getSecond_level_service_name(),  util);
                    if (iplist == null)
                    {
                        resp.setMessage("failed to get ip list for " + svc.getFirst_level_service_name()+"."+svc.getSecond_level_service_name());
                        resp.setStatus(100);
                        return resp;
                    }
                    c.setIp_count(iplist.size());
                    c.setFirst_level_service_name(svc.getFirst_level_service_name());
                    c.setSecond_level_service_name(svc.getSecond_level_service_name());

                    //获取每一个ip的负载信息
                    int sum = 0;
                    int count = 0;
                    int j;
                    for (j = 0; j < iplist.size() ; j++) {
                        IPPortPair ip = iplist.get(j);
                        String svcname = svc.getFirst_level_service_name()+"/"+svc.getSecond_level_service_name();
                        try {
                            int load = LoadBalance.readOneServiceLoadInfo(azk, svcname, ip.getIp());
                            if (load >= 0) {
                                sum += load;
                                count++;
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
                    c.setIp_report_num(count);
                    if (count > 0) {
                        c.setLoad_level(sum / count);
                    }
                    else
                    {
                        c.setLoad_level(0);
                    }
                    capacities.add(c);

                }
            }
            else
            {

                CapacityBaseInfo c = new CapacityBaseInfo();
                //该服务的所有ip
                ArrayList<IPPortPair> iplist = LoadBalance.getIPPortInfoByServiceName(request.getFirst_level_service_name(),
                        request.getSecond_level_service_name(), util);
                if (iplist == null)
                {
                    resp.setMessage("failed to get ip list for " + request.getFirst_level_service_name()+"."+request.getSecond_level_service_name());
                    resp.setStatus(100);
                    return resp;
                }
                c.setIp_count(iplist.size());
                c.setFirst_level_service_name(request.getFirst_level_service_name());
                c.setSecond_level_service_name(request.getSecond_level_service_name());

                //获取每一个ip的负载信息
                int sum = 0;
                int count = 0;
                int j;
                for (j = 0; j < iplist.size() ; j++) {
                    IPPortPair ip = iplist.get(j);
                    String svcname = request.getFirst_level_service_name()+"/"+request.getSecond_level_service_name();
                    try

                    {
                        int load = LoadBalance.readOneServiceLoadInfo(azk, svcname, ip.getIp());
                        if (load > 0) {
                            sum += load;
                            count++;
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
                c.setIp_report_num(count);
                if (count > 0) {
                    c.setLoad_level(sum / count);
                }
                else
                {
                    c.setLoad_level(0);
                }
                capacities.add(c);
            }

        }
        finally {
            util.releaseConn();
            azk.disconnect();
        }



        resp.setCapacity_list((ArrayList<CapacityBaseInfo>) capacities);
        resp.setMessage("success");
        resp.setStatus(0);

        return resp;


    }
}
