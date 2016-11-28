
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


package beans.service;

import org.apache.log4j.Logger;

import javax.servlet.ServletContext;
import java.util.ArrayList;

/**
 * Created by Administrator on 2016/2/16.
 */
public class ClusterProc implements Runnable {
    JedisHelper helper;
    ArrayList<String> ips;
    String operation;
    ServletContext servletContext;

    public ClusterProc(JedisHelper helper_, ArrayList<String> ips_, String operation_, ServletContext context)
    {
        helper = helper_;
        ips = ips_;
        operation = operation_;
        servletContext = context;
    }

    @Override
    public void run() {
        Logger logger = Logger.getLogger(ClusterProc.class);
        if(operation.equals("create")) {
            helper.CreateSet(ips);
        } else if (operation.equals("add")) {
            helper.AddSet(ips);
        } else if (operation.equals("remove")) {
            helper.RemoveSet(ips);
        } else if (operation.equals("recover")) {
            helper.RecoverSet(ips);
        }
    }
}
