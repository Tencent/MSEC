
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

import msec.org.JsonRPCHandler;
import msec.org.JsonRPCResponseBase;

import javax.servlet.ServletOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.PrintWriter;

/**
 * Created by Administrator on 2016/2/24.
 */
public class DownloadMonitorChart extends JsonRPCHandler {
    public JsonRPCResponseBase exec(MonitorRequest request)
    {
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            return null;
        }

        String filename = MonitorBySvcOrIP.getChartDirector()+ File.separator+request.getChart_to_download();
        try {
            File file = new File(filename);
            FileInputStream in = new FileInputStream(file);
            // set the MIME type.
            getHttpResponse().setContentType("image/png");
            getHttpResponse().setHeader("Content_Length", String.format("%d", file.length()));
            getHttpResponse().setHeader("Content-Disposition", "inline; filename=\"chart.png\"");

            ServletOutputStream out = getHttpResponse().getOutputStream();
            byte[] buf = new byte[10240];
            while (true)
            {
                int len = in.read(buf);
                if (len <= 0)
                {
                    break;
                }
                out.write(buf, 0, len);
            }
            out.close();
            in.close();
            //下载完就清理掉吧
            file.delete();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return null;
        }
        return null;
    }
}
