
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
import beans.dbaccess.ServerInfo;
import beans.request.RedisCmdRequest;
import beans.response.RedisCmdResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;
import redis.clients.jedis.Jedis;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Created by Administrator on 2016/1/25.
 * Redis Command
 */
public class RedisCmd extends JsonRPCHandler {

    public RedisCmdResponse exec(RedisCmdRequest request)
    {
        Logger logger = Logger.getLogger(RedisCmd.class);
        RedisCmdResponse resp = new RedisCmdResponse();
        String result = checkIdentity();
        String detail = "";
        if (!result.equals("success"))
        {
            resp.setStatus(99);
            resp.setMessage(result);
            return resp;
        }

        if(!request.getCommand().startsWith("cluster")) {
            try {
                String[] ip_pair = request.getHost().split(":");
                Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                jedis.connect();
                if(request.getCommand().equals("info")) {
                    detail = jedis.clusterInfo();
                    detail += jedis.info();
                    detail = detail.replace("\r","");
                    detail = detail.replace("\n", "<br/>");
                }
            }
            catch (Exception e) {
                logger.error("Exception", e);
                resp.setStatus(101);
                resp.setMessage(e.toString());
                return resp;
            }
        }
        else {
            if(request.getCommand().endsWith("failover")) {
                try {
                    String[] ip_pair = request.getHost().split(":");
                    JedisHelper helper = new JedisHelper("", ip_pair[0], Integer.parseInt(ip_pair[1]), 0);
                    HashMap<String, JedisHelper.ClusterStatus> status_map = helper.CheckStatus();
                    JedisHelper.ClusterStatus status = status_map.get(request.getHost());
                    logger.info(String.format("%b|%s|%s",status.isMaster(), status.getMaster_ip(), status.getMaster_nodeid()));
                    if(!status.isMaster()) {
                        helper.getCluster().get(request.getHost()).clusterFailover();
                        detail = "OK";
                    }
                }
                catch (Exception e) {
                    logger.error("Exception", e);
                    resp.setStatus(101);
                    resp.setMessage(e.toString());
                    return resp;
                }
            }

        }
        resp.setDetail(detail);
        resp.setMessage("success");
        resp.setStatus(0);
        return resp;
    }
}
