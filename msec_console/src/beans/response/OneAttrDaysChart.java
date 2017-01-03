
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

import ngse.org.JsonRPCResponseBase;
import ngse.org.Tools;

/**
 * Created by Administrator on 2016/2/25.
 */


public class OneAttrDaysChart extends JsonRPCResponseBase {
    public String getChart_file_name() {
        return chart_file_name;
    }

    public void setChart_file_name(String chart_file_name) {
        this.chart_file_name = chart_file_name;
    }

    public String getAttribute() {
        return attribute;
    }

    public void setAttribute(String attribute) {
        this.attribute = attribute;
    }

    public String getServer_ip() {
        return server_ip;
    }

    public void setServer_ip(String server_ip) {
        this.server_ip = server_ip;
    }

    public String getService_name() {
        return service_name;
    }

    public void setService_name(String service_name) {
        this.service_name = service_name;
    }

    public int getMax() {
        return max;
    }

    public void setMax(int max) {
        this.max = max;
    }

    public int getMin() {
        return min;
    }

    public void setMin(int min) {
        this.min = min;
    }

    public int getSum() {
        return sum;
    }

    public void setSum(int sum) {
        this.sum = sum;
    }


    public OneAttrDaysChart(String svc, String attr, String ip)
    {
        this.setService_name(svc);
        this.setAttribute(attr);
        this.setServer_ip(ip);
    }

    String chart_file_name;
    String attribute;
    String server_ip;
    String service_name;
    int max;
    int min;
    int sum;
}
