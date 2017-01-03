
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

import java.util.ArrayList;

/**
 * Created by Administrator on 2016/2/23.
 */
public class MonitorResponse extends JsonRPCResponseBase {

    Integer page_idx;
    Integer page_num;
    ArrayList<OneAttrChart> charts;
    String date;
    String compareDate;

    public String getDate() {
        return date;
    }

    public void setDate(String date) {
        this.date = date;
    }

    public String getCompareDate() {
        return compareDate;
    }

    public void setCompareDate(String compareDate) {
        this.compareDate = compareDate;
    }

    public Integer getPage_idx() {
        return page_idx;
    }

    public void setPage_idx(Integer page_idx) {
        this.page_idx = page_idx;
    }

    public Integer getPage_num() {
        return page_num;
    }

    public void setPage_num(Integer page_num) {
        this.page_num = page_num;
    }

    public ArrayList<OneAttrChart> getCharts() {
        return charts;
    }

    public void setCharts(ArrayList<OneAttrChart> charts) {
        this.charts = charts;
    }
}
