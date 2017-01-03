
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

import beans.request.QueryConfigInLBRequest;
import beans.request.IPPortPair;
import beans.response.QueryConfigInLBResponse;
import ngse.org.AccessZooKeeper;
import ngse.org.JsonRPCHandler;
import org.apache.log4j.Logger;
import org.json.JSONArray;
import org.json.JSONObject;

import java.nio.charset.Charset;
import java.util.ArrayList;

/**
 * Created by Administrator on 2016/3/7.
 */
public class QueryConfigInLB extends JsonRPCHandler {
    public QueryConfigInLBResponse exec(QueryConfigInLBRequest request)
    {
        QueryConfigInLBResponse response = new QueryConfigInLBResponse();
        Logger logger = Logger.getLogger(QueryConfigInLB.class);
        String svc = request.getFirst_level_service_name() + "/"+request.getSecond_level_service_name();
        AccessZooKeeper azk = new AccessZooKeeper();

        try {
            //读LB系统里的配置信息
            byte[] ret = LoadBalance.readOneServiceConfigInfo(azk, svc);
            if (ret == null)
            {
                response.setStatus(100);
                response.setMessage("read failed.");
                return response;
            }
            String s  = new String(ret, Charset.forName("UTF-8"));
            logger.error(s);

            //json格式解析
            ArrayList<IPPortPair> ip_list = new ArrayList<>();
            JSONObject obj = new JSONObject(s);
            JSONArray arr = obj.getJSONArray("IPInfo");
            for (int i = 0; i < arr.length(); ++i)//每一个IP
            {
                obj = arr.getJSONObject(i);

                JSONArray arr2 = obj.getJSONArray("ports");
                for (int j = 0; j < arr2.length(); j++) { //该IP上的每一个端口
                    IPPortPair pair = new IPPortPair();
                    pair.setPort(arr2.getInt(j));
                    pair.setIp(obj.getString("IP"));

                    ip_list.add(pair);
                }
            }
            response.setIp_list(ip_list);
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
        }
    }
}
