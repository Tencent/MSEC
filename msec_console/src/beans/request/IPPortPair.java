
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


package beans.request;

/**
 * Created by Administrator on 2016/3/2.
 */
public class IPPortPair {

    String ip;
    Integer port;
    String status;
    String release_memo;
    String comm_proto;

    public String getComm_proto() {
        return comm_proto;
    }

    public void setComm_proto(String comm_proto) {
        this.comm_proto = comm_proto;
    }

    public String getRelease_memo() {
        return release_memo;
    }

    public void setRelease_memo(String release_memo) {
        this.release_memo = release_memo;
    }

    public String getIp() {
        return ip;
    }

    public void setIp(String ip) {
        this.ip = ip;
    }

    public Integer getPort() {
        return port;
    }

    public void setPort(Integer port) {
        this.port = port;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }
}
