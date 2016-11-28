
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

/**
 * Created on 2016/5/16.
 */
public class ServerInfo {
    public String getSecond_level_service_name() {
        return second_level_service_name;
    }

    public void setSecond_level_service_name(String second_level_service_name) {
        this.second_level_service_name = second_level_service_name;
    }

    public String getFirst_level_service_name() {
        return first_level_service_name;
    }

    public void setFirst_level_service_name(String first_level_service_name) {
        this.first_level_service_name = first_level_service_name;
    }

    public String getIp() {
        return ip;
    }

    public void setIp(String ip) {
        this.ip = ip;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }

    public String getSet_id() {
        return set_id;
    }

    public void setSet_id(String set_id) {
        this.set_id = set_id;
    }

    public int getGroup_id() {
        return group_id;
    }

    public void setGroup_id(int group_id) {
        this.group_id = group_id;
    }

    public int getMemory() {
        return memory;
    }

    public void setMemory(int memory) {
        this.memory = memory;
    }

    public boolean isMaster() {
        return master;
    }

    public void setMaster(boolean master) {
        this.master = master;
    }

    public String getRecover_host() {
        return recover_host;
    }

    public void setRecover_host(String recover_host) {
        this.recover_host = recover_host;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public String getOperation() {
        return operation;
    }

    public void setOperation(String operation) {
        this.operation = operation;
    }

    public ServerInfo() {};

    public ServerInfo(ServerInfo info) {
        second_level_service_name = info.getSecond_level_service_name();
        first_level_service_name = info.getFirst_level_service_name();
        ip = info.getIp();
        port = info.getPort();
        set_id = info.getSet_id();
        group_id = info.getGroup_id();
        memory = info.getMemory();
        master = info.isMaster();
        status = info.getStatus();
        operation = info.getOperation();
        recover_host = info.getRecover_host();
    }


    public String toString() {
        return ip + ":" + port + "@" + set_id + ":" + master;
    }

    String second_level_service_name;
    String first_level_service_name;
    String ip;      //IP
    int port;       //端口
    String set_id;     //Set ID
    int group_id;   //组ID
    int memory;     //最大使用内存
    boolean master;  //是否主机
    String status;  //安装进度
    String operation;   //操作类型
    String recover_host;    //only for recover logic
}
