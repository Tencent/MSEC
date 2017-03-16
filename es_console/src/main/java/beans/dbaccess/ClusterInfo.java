
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


package beans.dbaccess;

import java.util.ArrayList;

public class ClusterInfo {
    ArrayList<String> req_ips;
    int req_port;
    ArrayList<ServerInfo> server_list;
    String first_level_service_name;
    String second_level_service_name;
    String plan_id;

    public String getCluster_name() {
        return first_level_service_name + "." + second_level_service_name;
    }

    public ArrayList<String> getReq_ips() {
        return req_ips;
    }

    public void setReq_ips(ArrayList<String> req_ips) {
        this.req_ips = req_ips;
    }

    public int getReq_port() {
        return req_port;
    }

    public void setReq_port(int req_port) {
        this.req_port = req_port;
    }

    public ArrayList<ServerInfo> getServer_list() {
        return server_list;
    }

    public void setServer_list(ArrayList<ServerInfo> server_list) {
        this.server_list = server_list;
    }

    public String getFirst_level_service_name() {
        return first_level_service_name;
    }

    public void setFirst_level_service_name(String first_level_service_name) {
        this.first_level_service_name = first_level_service_name;
    }

    public String getSecond_level_service_name() {
        return second_level_service_name;
    }

    public void setSecond_level_service_name(String second_level_service_name) {
        this.second_level_service_name = second_level_service_name;
    }

    public String getPlan_id() {
        return plan_id;
    }

    public void setPlan_id(String plan_id) {
        this.plan_id = plan_id;
    }
}
