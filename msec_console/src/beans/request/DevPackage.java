
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

/**
 * Created by Administrator on 2016/2/1.
 */
public class DevPackage {

    String first_level_service_name;
    String second_level_service_name;
    String config_tag;
    String idl_tag;
    String sharedobject_tag;
    Integer step_number;
    String dev_lang;

    public String getDev_lang() {
        return dev_lang;
    }

    public void setDev_lang(String dev_lang) {
        this.dev_lang = dev_lang;
    }

    public Integer getStep_number() {
        return step_number;
    }

    public void setStep_number(Integer step_number) {
        this.step_number = step_number;
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
