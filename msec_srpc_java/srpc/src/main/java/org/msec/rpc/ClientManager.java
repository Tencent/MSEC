
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


package org.msec.rpc;

import api.lb.msec.org.AccessLB;
import api.lb.msec.org.Route;
import api.monitor.msec.org.AccessMonitor;
import org.apache.log4j.Logger;
import org.msec.net.NettyClient;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;


public class ClientManager {
    private static Logger log = Logger.getLogger(ClientManager.class.getName());

    private static Map<Route, NettyClient> clientCache = new ConcurrentHashMap<Route, NettyClient>();
    private static AccessLB accessLB = new AccessLB();

    public static NettyClient  getClient(String serviceName, boolean fromCache) throws Exception {
        Route route = new Route();
        /*route.setIp("127.0.0.1");
        route.setPort(7963);
        route.setComm_type(Route.COMM_TYPE.COMM_TYPE_TCP);
        */

        if (!accessLB.getroutebyname(serviceName, route)) {
            log.error("getRouteByName failed: " + serviceName);
            AccessMonitor.add("frm.rpc route to " + serviceName + " failed");
            return null;
        }

        AccessMonitor.add("frm.rpc route to " + serviceName + " succ.");

        NettyClient client = null;
        if (fromCache) {
            client = clientCache.get(route);
        }
        log.info("getRouteByName " + serviceName + " addr: " + route.getIp() + ":" + route.getPort() + " incache: " + (client != null) +
                 " connected: " + ((client != null) && client.isConnected()));
        if (client == null) {
            client = new NettyClient(route.getIp(), route.getPort());
            clientCache.put(route, client);
        }

        if (!client.isConnected()) {
            client.connect();
            AccessMonitor.add("frm.rpc new tcp connection");
        }

        if (!client.isConnected()) {
            AccessMonitor.add("frm.rpc new tcp connection failed.");
            throw new RuntimeException("Connection failure to " + route.getIp() + ":" + route.getPort());
        }

        return client;
    }

}
