
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

import beans.request.IPPortPair;
import ngse.org.JsonRPCResponseBase;

import java.util.ArrayList;

/**
 * Created by Administrator on 2016/3/7.
 */
public class QueryConfigInLBResponse extends JsonRPCResponseBase {
    ArrayList<IPPortPair> ip_list;

    public ArrayList<IPPortPair> getIp_list() {
        return ip_list;
    }

    public void setIp_list(ArrayList<IPPortPair> ip_list) {
        this.ip_list = ip_list;
    }
}
