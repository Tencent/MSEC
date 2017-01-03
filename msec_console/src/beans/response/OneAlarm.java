
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



/**
 * Created by Administrator on 2016/4/27.
 */
public class OneAlarm {

    String service_name;
    String attribute_name;
    String occur_date;
    String last_occur_time;
    int last_occur_unix_timestamp;
    int alarm_type;
    int threshold;

    public int getThreshold() {
        return threshold;
    }

    public void setThreshold(int threshold) {
        this.threshold = threshold;
    }

    public int getLast_occur_unix_timestamp() {
        return last_occur_unix_timestamp;
    }

    public void setLast_occur_unix_timestamp(int last_occur_unix_timestamp) {
        this.last_occur_unix_timestamp = last_occur_unix_timestamp;
    }

    public String getService_name() {
        return service_name;
    }

    public void setService_name(String service_name) {
        this.service_name = service_name;
    }

    public String getAttribute_name() {
        return attribute_name;
    }

    public void setAttribute_name(String attribute_name) {
        this.attribute_name = attribute_name;
    }

    public String getOccur_date() {
        return occur_date;
    }

    public void setOccur_date(String occur_date) {
        this.occur_date = occur_date;
    }

    public int getAlarm_type() {
        return alarm_type;
    }

    public void setAlarm_type(int alarm_type) {
        this.alarm_type = alarm_type;
    }

    public String getLast_occur_time() {
        return last_occur_time;
    }

    public void setLast_occur_time(String last_occur_time) {
        this.last_occur_time = last_occur_time;
    }
}
