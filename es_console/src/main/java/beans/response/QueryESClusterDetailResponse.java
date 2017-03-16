
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


package beans.response;

import beans.service.QueryESClusterDetail;
import msec.org.JsonRPCResponseBase;

import java.util.ArrayList;
import java.util.HashMap;

/**
 *
 */
public class QueryESClusterDetailResponse extends JsonRPCResponseBase {
    public class RTInfo {
        public RTInfo() {
            OK = false;
            doc_count = 0;
            doc_disk_size = 0;
            avail_disk_size = 0;
        }

        boolean OK;
        long doc_count;
        long doc_disk_size;
        long avail_disk_size;

        public boolean isOK() {
            return OK;
        }

        public void setOK(boolean OK) {
            this.OK = OK;
        }

        public long getDoc_count() {
            return doc_count;
        }

        public void setDoc_count(long doc_count) {
            this.doc_count = doc_count;
        }

        public long getDoc_disk_size() {
            return doc_disk_size;
        }

        public void setDoc_disk_size(long doc_disk_size) {
            this.doc_disk_size = doc_disk_size;
        }

        public long getAvail_disk_size() {
            return avail_disk_size;
        }

        public void setAvail_disk_size(long avail_disk_size) {
            this.avail_disk_size = avail_disk_size;
        }
    }

    public QueryESClusterDetailResponse() {
        health_status = "INIT";
        info_map = new HashMap<>();
        active_shards = 0;
        total_shards = 0;
        server_port = 0;
    }

    HashMap<String, RTInfo> info_map;
    String health_status;
    int active_shards;
    int total_shards;
    int server_port;

    public HashMap<String, RTInfo> getInfo_map() {
        return info_map;
    }

    public void setInfo_map(HashMap<String, RTInfo> info_map) {
        this.info_map = info_map;
    }

    public String getHealth_status() {
        return health_status;
    }

    public void setHealth_status(String health_status) {
        this.health_status = health_status;
    }

    public int getActive_shards() {
        return active_shards;
    }

    public void setActive_shards(int active_shards) {
        this.active_shards = active_shards;
    }

    public int getTotal_shards() {
        return total_shards;
    }

    public void setTotal_shards(int total_shards) {
        this.total_shards = total_shards;
    }

    public int getServer_port() {
        return server_port;
    }

    public void setServer_port(int server_port) {
        this.server_port = server_port;
    }
}
