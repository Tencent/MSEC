
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

import beans.request.MonitorRequest;
import beans.response.OneAttrDaysChart;
import beans.response.OneDayValue;
import ngse.org.JsonRPCHandler;
import ngse.org.Tools;
import org.apache.log4j.Logger;
import java.util.ArrayList;

/**
 * Created by Administrator on 2016/4/28.
 */
public class MonitorAttrContDays extends JsonRPCHandler {


    public OneAttrDaysChart exec(MonitorRequest request)
    {
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            return null;
        }

        Logger logger = Logger.getLogger(MonitorAttrContDays.class);

        OneAttrDaysChart chart = new OneAttrDaysChart(request.getService_name(), request.getAttribute(), request.getServer_ip());
        try {

            ArrayList<String> dateList = new ArrayList<String>();
            for(int i = request.getDuration()-1;i>=0;i--) {
                dateList.add(Tools.getPreviousDate(request.getDate(),i));
            }

            ArrayList<OneDayValue> values = MonitorBySvcOrIP.getAttrDaysValue(request.getService_name(),
                    request.getAttribute(),
                    request.getServer_ip(),
                    dateList);
            if(request.getServer_ip()!=null && request.getServer_ip().length() > 0)
                MonitorBySvcOrIP.drawDaysChart(chart, values, request.getServer_ip(), request.getDuration());
            else
                MonitorBySvcOrIP.drawDaysChart(chart, values, request.getAttribute(), request.getDuration());
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return null;
        }
        return chart;
    }
}
