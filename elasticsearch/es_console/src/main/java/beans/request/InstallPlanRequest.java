
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

import beans.dbaccess.ServerInfo;
import beans.response.AddSecondLevelServiceIPInfoResponse;

import java.util.ArrayList;

public class InstallPlanRequest {
    String first_level_service_name;
    String second_level_service_name;
    ArrayList<String> added_ips;
    String data_dir;

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

    public ArrayList<String> getAdded_ips() {
        return added_ips;
    }

    public void setAdded_ips(ArrayList<String> added_ips) {
        this.added_ips = added_ips;
    }

    public String getData_dir() {
        return data_dir;
    }

    public void setData_dir(String data_dir) {
        this.data_dir = data_dir;
    }
}
