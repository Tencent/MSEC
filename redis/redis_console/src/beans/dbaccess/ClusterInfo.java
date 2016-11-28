
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

/**
 * Created on 2016/6/29.
 */
public class ClusterInfo {
    int copy_num;
    int memory_per_instance;
    String plan_id;
    ArrayList<String> ip_port_list;

    public int getCopy_num() {
        return copy_num;
    }

    public void setCopy_num(int copy_num) {
        this.copy_num = copy_num;
    }

    public int getMemory_per_instance() {
        return memory_per_instance;
    }

    public void setMemory_per_instance(int memory_per_instance) {
        this.memory_per_instance = memory_per_instance;
    }

    public String getPlan_id() {
        return plan_id;
    }

    public void setPlan_id(String plan_id) {
        this.plan_id = plan_id;
    }

    public ArrayList<String> getIp_port_list() {
        return ip_port_list;
    }

    public void setIp_port_list(ArrayList<String> ip_port_list) {
        this.ip_port_list = ip_port_list;
    }
}
