
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
 * Created by Administrator on 2016/1/27.
 */
public class DBAnalyzeInfo {
    Long record_number;
    Integer max_integer;
    Integer min_integer;

    public Long getRecord_number() {
        return record_number;
    }

    public void setRecord_number(Long record_number) {
        this.record_number = record_number;
    }

    public Integer getMax_integer() {
        return max_integer;
    }

    public void setMax_integer(Integer max_integer) {
        this.max_integer = max_integer;
    }

    public Integer getMin_integer() {
        return min_integer;
    }

    public void setMin_integer(Integer min_integer) {
        this.min_integer = min_integer;
    }
}
