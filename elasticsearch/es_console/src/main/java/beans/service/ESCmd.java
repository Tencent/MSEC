
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

import beans.request.ESCmdRequest;
import beans.response.ESCmdResponse;
import msec.org.JsonRPCHandler;
import msec.org.RemoteShell;
import org.apache.log4j.Logger;

public class ESCmd extends JsonRPCHandler {

    public ESCmdResponse exec(ESCmdRequest request)
    {
        Logger logger = Logger.getLogger(ESCmd.class);
        ESCmdResponse resp = new ESCmdResponse();
        String result = checkIdentity();
        String result_message = "";
        if (!result.equals("success"))
        {
            resp.setStatus(99);
            resp.setMessage(result);
            return resp;
        }

        logger.info(request.getCommand());
        if(request.getCommand().equals("restart")) {
            RemoteShell remoteShell = new RemoteShell();
            String ip = request.getHost().split(":")[0];
            String cmd =
                    "MSG=`/data/stop.sh | grep -v ok`\n" +
                    "if [ ! -z \"$MSG\" ]; then echo $MSG; exit 1; fi\n" +
                    "sleep 2;/data/start.sh | grep -v ok\n";
            StringBuffer output = new StringBuffer();
            result = remoteShell.SendCmdsToRunAndGetResultBack(cmd, ip, output);
            if (result == null || !result.equals("success")) {
                logger.error(String.format("remote error:%s|%s", ip, result));
                resp.setStatus(100);
                resp.setMessage("Restart fails.");
            }
            else {
                logger.info(String.format("%s|%s|%s", ip, output, cmd));
                if (!output.toString().isEmpty()) {
                    logger.error(String.format("restart es node error:%s|%s", ip, output));
                    result_message="[ERROR] " +output.toString();
                } else {
                    result_message="Server restarts.";
                }
            }
        }
        resp.setResult(result_message);
        resp.setMessage("success");
        resp.setStatus(0);
        return resp;
    }
}
