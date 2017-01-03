
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
 * Created by Administrator on 2016/2/9.
 */
public class CapacityBaseInfo {
    String first_level_service_name;
    String second_level_service_name;
    Integer ip_count;//配置的IP个数
    Integer ip_report_num;//有负载和心跳上报的IP个数
    Integer load_level;



    public Integer getLoad_level() {
        return load_level;
    }

    public void setLoad_level(Integer load_level) {
        this.load_level = load_level;
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

    public Integer getIp_count() {
        return ip_count;
    }

    public void setIp_count(Integer ip_count) {
        this.ip_count = ip_count;
    }

    public Integer getIp_report_num() {
        return ip_report_num;
    }

    public void setIp_report_num(Integer ip_report_num) {
        this.ip_report_num = ip_report_num;
    }
}
