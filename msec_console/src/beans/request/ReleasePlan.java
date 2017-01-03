
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

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;
import java.util.Random;

/**
 * Created by Administrator on 2016/2/1.
 */
public class ReleasePlan {



    String first_level_service_name;
    String second_level_service_name;

    ArrayList<IPPortPair> dest_ip_list;
    String config_tag;
    String idl_tag;
    String sharedobject_tag;
    String plan_id;
    Integer step_number;
    String status;
    String memo;
    String backend_task_status;
    String release_type;
    String dev_lang;

    public String getDev_lang() {
        return dev_lang;
    }

    public void setDev_lang(String dev_lang) {
        this.dev_lang = dev_lang;
    }

    public String getRelease_type() {
        return release_type;
    }

    public void setRelease_type(String release_type) {
        this.release_type = release_type;
    }

    public String getBackend_task_status() {
        return backend_task_status;
    }

    public void setBackend_task_status(String backend_task_status) {
        this.backend_task_status = backend_task_status;
    }

    public String getMemo() {
        return memo;
    }

    public void setMemo(String memo) {
        this.memo = memo;
    }

    public String getStatus() {
        return status;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public Integer getStep_number() {
        return step_number;
    }

    public void setStep_number(Integer step_number) {
        this.step_number = step_number;
    }

    public static String newPlanID()
    {
        double r = Math.random();
        SimpleDateFormat df = new SimpleDateFormat("yyyyMMddHHmmss_");//设置日期格式
        String ret = df.format(new Date());
        ret = ret + (int)(Integer.MAX_VALUE * r);
        return ret;
    }

    public String getPlan_id() {
        return plan_id;
    }

    public void setPlan_id(String plan_id) {
        this.plan_id = plan_id;
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

    public ArrayList<IPPortPair> getDest_ip_list() {
        return dest_ip_list;
    }

    public void setDest_ip_list(ArrayList<IPPortPair> dest_ip_list) {
        this.dest_ip_list = dest_ip_list;
    }



    public String getConfig_tag() {
        return config_tag;
    }

    public void setConfig_tag(String config_tag) {
        this.config_tag = config_tag;
    }

    public String getIdl_tag() {
        return idl_tag;
    }

    public void setIdl_tag(String idl_tag) {
        this.idl_tag = idl_tag;
    }

    public String getSharedobject_tag() {
        return sharedobject_tag;
    }

    public void setSharedobject_tag(String sharedobject_tag) {
        this.sharedobject_tag = sharedobject_tag;
    }
}
