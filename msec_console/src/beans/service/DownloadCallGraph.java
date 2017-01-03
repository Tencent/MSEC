
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

import beans.request.Alarm;
import beans.request.BusinessLog;
import beans.response.AlarmResponse;
import beans.response.OneAttrChart;
import ngse.org.JsonRPCHandler;
import ngse.org.JsonRPCResponseBase;
import org.apache.log4j.Logger;

import javax.servlet.ServletOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;

/**
 * Created by Administrator on 2016/4/28.
 */
public class DownloadCallGraph extends JsonRPCHandler {
    public JsonRPCResponseBase exec(BusinessLog request)
    {
        Logger logger = Logger.getLogger(DownloadCallGraph.class);

        try {
            String filename = QueryBusinessLog.getGraphFilename(request.getRequest_id());


            File file = new File(filename);
            FileInputStream in = new FileInputStream(file);
            // set the MIME type.
            getHttpResponse().setContentType("image/png");
            getHttpResponse().setHeader("Content_Length", String.format("%d", file.length()));


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
