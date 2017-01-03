
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
public class RollbackReleasePlan extends JsonRPCHandler {
    private String plan_id;
    private String flsn;
    private String slsn;
    private String release_type;
    private String release_memo;



    private String geneCmdFileForRollback()
    {
       String fmt =
               "if [ -e /msec/backup/backup_%s ]; then\n" +
               " cd /msec/%s/%s/bin/; chmod a+x ./*; ./srpc.sh stop;\n"+
               " rm -rf /msec/%s/%s/*\n"+
               " mkdir -p /msec/%s/%s\n"+
               " cp -R /msec/backup/backup_%s/* /msec/%s/%s/\n"+
               " cd /msec/%s/%s/bin/; chmod a+x ./*; ./srpc.sh start;\n"+
               "fi\n";


        String cmdFileName = ServletConfig.fileServerRootDir+"/tmp/"+ plan_id+".sh";
        String content = String.format(fmt,
                plan_id,
                flsn, slsn,//cd
                flsn, slsn, //rm
                flsn, slsn, //mkdir -p
                plan_id, flsn, slsn, //cp -R
                flsn, slsn); //cd
        content =  content+CarryOutReleasePlan.showProcessCmds(flsn, slsn);

        try {
            FileOutputStream out = new FileOutputStream(cmdFileName);
            out.write(content.getBytes());
            out.close();
        }
        catch ( Exception e)
        {
            e.printStackTrace();
            return "";
        }
        return cmdFileName;

    }
    //safeWrite能够避免因为前端浏览器关闭了连接而导致发布没有执行下去
    private void safeWrite(String s, ServletOutputStream out)
    {
        try
        {
            out.println(s);
            out.flush();
        }
        catch (Exception e){}
    }
    private void safeWrite(byte[] b,int offset, int len,  ServletOutputStream out)
    {
        try
        {
            out.write(b, offset, len);
            out.flush();

        }
        catch (Exception e){}
    }



    private void updateReleaseMemo(DBUtil util, IPPortPair ipPortPair) throws Exception
    {
        String sql = "update  t_second_level_service_ipinfo set release_memo=? where ip=? and port=?";
        ArrayList<Object> params = new ArrayList<Object>();
        params.add("roll back");
        params.add(ipPortPair.getIp());
        params.add(ipPortPair.getPort());

        util.updateByPreparedStatement(sql, params);
    }

    private void doRollback(IPPortPair[] ips,
                           ServletOutputStream out,
                           DBUtil util) throws Exception
    {
        Logger logger = Logger.getLogger(RollbackReleasePlan.class);

        RemoteShell remoteShell = new RemoteShell();

        String cmdFile = geneCmdFileForRollback();


        logger.info("cmd file:" + cmdFile);


        byte[] buf = new byte[10240];

        for (int i = 0; i < ips.length; i++) {
            String ip = ips[i].getIp();

            String result = "";

            StringBuffer outputFileName = new StringBuffer();
            result = remoteShell.SendCmdsToAgentAndRun(cmdFile, ip, outputFileName);
            if (result == null || !result.equals("success"))
            {
                safeWrite(String.format(">>>send cmd file to %s failed:%s", ip, result), out);
                continue;
            }
            safeWrite(String.format("send cmd file to %s succesfully", ip), out);


            String localResult = ServletConfig.fileServerRootDir+"/tmp/" + plan_id+".out";
            logger.info("run cmd file,get result file:"+localResult);

            result = remoteShell.GetFileFromAgent(localResult, outputFileName.toString(), ip);
            if (result == null || !result.equals("success"))
            {
               safeWrite(String.format(">>>get result file from %s failed:%s\n", ip, result), out);
                continue;
            }
            safeWrite(String.format("get result file from %s succesfully\n", ip), out);

            FileInputStream in = new FileInputStream(localResult);

            while (true)
            {
                int len = in.read(buf);
                if (len < 0)
                {
                    break;
                }
                safeWrite(buf, 0, len, out);
            }
            in.close();
            safeWrite("--------------------------", out);

            //更新该IP的版本信息
            updateReleaseMemo(util, ips[i]);
        }
        out.println("done!");
    }
    private void updatePlanStatus(DBUtil util, String status) throws  Exception
    {
        String sql = "update t_release_plan set status=? where plan_id=?";
        ArrayList<Object> params = new ArrayList<Object>();
        params.add(status);
        params.add(plan_id);

        util.updateByPreparedStatement(sql, params);
    }
    public JsonRPCResponseBase exec(ReleasePlan request)
    {
        Logger logger = Logger.getLogger(RollbackReleasePlan.class);

        JsonRPCResponseBase resp = new JsonRPCResponseBase();
        plan_id = request.getPlan_id();
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

        try {


            String sql = "select dest_ip_list from t_release_plan where plan_id=?";
            ArrayList<Object> params = new ArrayList<Object>();
            params.add(request.getPlan_id());

            Map<String, Object> res = util.findSimpleResult(sql,params);
            if (res.get("dest_ip_list") == null )
            {
                resp.setStatus(100);
                resp.setMessage("query dest ip list failed.");
                return resp;
            }
            String dest_ip_list = (String)(res.get("dest_ip_list"));

            ObjectMapper objectMapper = new ObjectMapper();
            IPPortPair[] ips = objectMapper.readValue(dest_ip_list, IPPortPair[].class);

            logger.info("destination IP number;"+ips.length);





            //对每个IP执行回滚

            getHttpResponse().setContentType("text/html");
            getHttpResponse().setCharacterEncoding("UTF-8");
            ServletOutputStream out = getHttpResponse().getOutputStream();

            safeWrite("<html>", out);
            safeWrite("<head>", out);
            safeWrite("<title>release result</title>", out);
            safeWrite("</head>", out);
            safeWrite("<body>", out);
            safeWrite("<pre>", out);//这个标签用于原样输出

            updatePlanStatus(util, "rolling back");

            doRollback( ips, out, util);


            safeWrite("</pre>", out);
            safeWrite("</body>", out);
            safeWrite("</html>", out);

            //update plan status
            updatePlanStatus(util, "roll back successfully");


            try {out.close();}catch (Exception e){}
            return null;



        }
        catch (Exception e)
        {
            resp.setStatus(100);
            resp.setMessage(e.getMessage());
            e.printStackTrace();
            try {
                updatePlanStatus(util, "failed to roll back");
            }
            catch (Exception e1){}
            return resp;
        }
        finally {
            util.releaseConn();
        }


    }
}
