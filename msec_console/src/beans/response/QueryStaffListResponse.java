
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

import beans.dbaccess.StaffInfo;
import ngse.org.JsonRPCResponseBase;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/25.
 */
public class QueryStaffListResponse extends JsonRPCResponseBase {
    public ArrayList<StaffInfo> staff_list;

    public ArrayList<StaffInfo> getStaff_list() {
        return staff_list;
    }

    public void setStaff_list(ArrayList<StaffInfo> staff_list) {
        this.staff_list = staff_list;
    }
}
