
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

import beans.dbaccess.SecondLevelService;
import beans.request.IPPortPair;
import beans.request.ReleasePlan;
import ngse.org.*;
import org.apache.log4j.Logger;
import org.codehaus.jackson.map.ObjectMapper;

import javax.servlet.ServletOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Date;
import java.util.Map;


/**
 * Created by Administrator on 2016/2/2.
 * 查询某个发布计划的详细信息
 */
public class QueryProcessInfo extends JsonRPCHandler {
    private String flsn;
    private String slsn;



    private String getLocalResultFileName()
    {
        String  rnd = Tools.randInt();
        String fn = ServletConfig.fileServerRootDir+"/tmp/show_process_"+ rnd+".result";
        return fn;
    }

    private String geneCmdFile() throws Exception
    {
        String  rnd = Tools.randInt();
        String cmdFileName = ServletConfig.fileServerRootDir+"/tmp/show_process_"+ rnd+".sh";

        String content = CarryOutReleasePlan.showProcessCmds(flsn, slsn);

        new File(cmdFileName).delete();
        FileOutputStream out = new FileOutputStream(cmdFileName);
        out.write(content.getBytes());
        out.close();


        return cmdFileName;

    }


    public JsonRPCResponseBase exec(SecondLevelService request)
    {
        Logger logger = Logger.getLogger(QueryProcessInfo.class);

        JsonRPCResponseBase resp = new JsonRPCResponseBase();
        flsn = request.getFirst_level_service_name();
        slsn = request.getSecond_level_service_name();

        String result = checkIdentity();
        if (!result.equals("success"))
        {
            resp.setStatus(99);
            resp.setMessage(result);
            return resp;
        }
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            resp.setStatus(100);
            resp.setMessage("db connect failed!");
            return resp;
        }
        FileInputStream in = null;

        try {


            String sql = "select   ip from t_second_level_service_ipinfo where first_level_service_name=? "+
                    "and second_level_service_name=? and status='enabled' ";
            ArrayList<Object> params = new ArrayList<Object>();
            params.add(flsn);
            params.add(slsn);

            ArrayList<IPPortPair> iplist = util.findMoreRefResult(sql, params, IPPortPair.class);

            String cmdFileName = geneCmdFile();


            getHttpResponse().setContentType("text/html");
            getHttpResponse().setCharacterEncoding("UTF-8");
            ServletOutputStream out = getHttpResponse().getOutputStream();

            out.println("<html>");
            out.println("<head>");
            out.println("<title>release result</title>");
            out.println("</head>");
            out.println("<body>");
            out.println("<pre>");//这个标签用于原样输出

            StringBuffer outFile = new StringBuffer();
            String localResultFileName = getLocalResultFileName();
            new File(localResultFileName).delete();
            byte[] buffer = new byte[1240];

            for (int i = 0; i < iplist.size(); i++) {
                String ip = iplist.get(i).getIp();
                out.println(String.format("------------------IP:%s---------------", ip));
                RemoteShell rs = new RemoteShell();
                outFile.delete(0, outFile.length());
                result = rs.SendCmdsToAgentAndRun(cmdFileName, ip, outFile);
                if (result == null || !result.equals("success"))
                {
                    out.println(String.format("sending cmd file to %s failed:%s",
                            ip, result));
                    continue;
                }
                result = rs.GetFileFromAgent(localResultFileName, outFile.toString(), ip);
                if (result == null || !result.equals("success"))
                {
                    out.println(String.format("get cmd result from %s failed:%s",
                            ip, result));
                    continue;
                }
                in = new FileInputStream(localResultFileName);

                while (true)
                {
                    int len = in.read(buffer);
                    if (len <= 0)
                    {
                        break;
                    }
                    out.print(new String(buffer, 0, len));
                }
                in.close();
                new File(localResultFileName).delete();



            }


            out.println("</pre>");
            out.println("</body>");
            out.println("</html>");



            try {out.close();}catch (Exception e){}
            return null;



        }
        catch (Exception e)
        {
            resp.setStatus(100);
            resp.setMessage(e.getMessage());
            e.printStackTrace();
            return resp;
        }
        finally {
            util.releaseConn();
            if (in != null )
            {
                try {in.close();} catch (Exception e ){}
            }
        }


    }
}
