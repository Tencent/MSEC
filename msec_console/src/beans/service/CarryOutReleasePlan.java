
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
import java.io.*;
import java.util.ArrayList;
import java.util.Date;
import java.util.Map;


/**
 * Created by Administrator on 2016/2/2.
 * 查询某个发布计划的详细信息
 */
public class CarryOutReleasePlan extends JsonRPCHandler {
    private String plan_id;
    private String flsn;
    private String slsn;
    private String release_type;
    private String release_memo;

    public static  String showProcessCmds(String flsn, String slsn) {
        String ps = "\nsleep 1;echo 'ps|grep  srpc...'\n" +
                "ps auxw|grep srpc_%s_%s|grep -v grep;echo ''\n" +
                "echo 'ps |grep agent...'\n" +
                "ps auxw|grep -e monitor_agent -e nlbagent -e flume -e remote_shell_agent |grep -v grep\n";
        return String.format(ps, flsn, slsn);
    }

    //备份起来，用于回滚
    private String backupCmdString( boolean isFirstTimeFlag)
    {

        String s = "";
        if (isFirstTimeFlag) {
            s = String.format("mkdir -p /msec/backup/\n" +
                            "if [ -e '/msec/%s/%s' ] ; then\n" +
                            "  cp -R /msec/%s/%s/  /msec/backup/backup_%s\n" +
                            "fi\n",
                    flsn, slsn,
                    flsn, slsn, plan_id);
        }

        return s;
    }

    private String geneCmdFileForOnlyConfigRelease(String remoteFileFullName, String remoteFileBaseName, boolean isFirstTimeFlag)
    {
        String fmt = "rm /msec/%s/%s/etc/config.ini\n"+
                "cd /tmp; tar zxf %s\n" +
                "rm -rf /tmp/framework; mv /tmp/%s /tmp/framework\n"+
                "cp /tmp/framework/etc/config.ini /msec/%s/%s/etc/\n"+
                "cd /msec/%s/%s/bin;chmod a+x ./*; ./srpc.sh stop; ./srpc.sh start";




        String cmdFileName = ServletConfig.fileServerRootDir+"/tmp/"+ plan_id+".sh";
        String content = String.format(fmt,
                flsn, slsn, //rm
                remoteFileBaseName, //tar xf
                plan_id,// rm -rf
                flsn, slsn,//cp
                flsn, slsn);//restart
        content = backupCmdString(isFirstTimeFlag) +  content+showProcessCmds(flsn,slsn);
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
    private String geneCmdFileForOnlyLibraryRelease(String remoteFileFullName, String remoteFileBaseName, boolean isFirstTimeFlag)
    {
        String fmt = "rm -rf /msec/%s/%s/bin/lib\n"+
                "cd /tmp; tar zxf %s\n" +
                "rm -rf /tmp/framework; mv /tmp/%s /tmp/framework\n"+
                "cp -R /tmp/framework/bin/lib /msec/%s/%s/bin/\n"+
                "cd /msec/%s/%s/bin;chmod a+x ./*; ./srpc.sh stop;./srpc.sh start";


        String cmdFileName = ServletConfig.fileServerRootDir+"/tmp/"+ plan_id+".sh";
        String content = String.format(fmt,
                flsn, slsn, //rm
                remoteFileBaseName, //tar xf
                plan_id,// rm -rf
                flsn, slsn,//cp
                flsn, slsn);//restart
        content = backupCmdString(isFirstTimeFlag) + content+showProcessCmds(flsn,slsn);
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
    private String geneCmdFileForOnlySharedobjectRelease(String remoteFileFullName, String remoteFileBaseName, boolean isFirstTimeFlag, String dev_lang)
    {

        String fmt = "";
        //主要差异在第四行：业务插件的名字
        if (dev_lang.equals("c++")) {
            fmt = "rm /msec/%s/%s/bin/msec.so\n" +
                    "cd /tmp; tar zxf %s\n" +
                    "rm -rf /tmp/framework; mv /tmp/%s /tmp/framework\n" +
                    "cp /tmp/framework/bin/msec.so /msec/%s/%s/bin\n" +
                    "cd /msec/%s/%s/bin;chmod a+x ./*; ./srpc.sh stop; ./srpc.sh start";
        }
        else if  (dev_lang.equals("java"))
        {
            fmt = "rm /msec/%s/%s/bin/msec.jar\n" +
                    "cd /tmp; tar zxf %s\n" +
                    "rm -rf /tmp/framework; mv /tmp/%s /tmp/framework\n" +
                    "cp /tmp/framework/bin/msec.jar /msec/%s/%s/bin\n" +
                    "cd /msec/%s/%s/bin;chmod a+x ./*; ./srpc.sh stop; ./srpc.sh start";
        }
        else if (dev_lang.equals("php"))
        {
            fmt = "rm /msec/%s/%s/bin/msec.tgz\n" +
                    "cd /tmp; tar zxf %s\n" +
                    "rm -rf /tmp/framework; mv /tmp/%s /tmp/framework\n" +
                    "cp /tmp/framework/bin/msec.tgz /msec/%s/%s/bin\n" +
                    "cd /msec/%s/%s/bin;chmod a+x ./*; ./srpc.sh stop; ./srpc.sh start";
        }
        else if (dev_lang.equals("python"))
        {
            fmt = "rm /msec/%s/%s/bin/msec_py.tgz\n" +
                    "cd /tmp; tar zxf %s\n" +
                    "rm -rf /tmp/framework; mv /tmp/%s /tmp/framework\n" +
                    "cp /tmp/framework/bin/msec_py.tgz /msec/%s/%s/bin\n" +
                    "cd /msec/%s/%s/bin;chmod a+x ./*; ./srpc.sh stop; ./srpc.sh start";
        }


        String cmdFileName = ServletConfig.fileServerRootDir+"/tmp/"+ plan_id+".sh";
        String content = String.format(fmt,
                flsn, slsn, //rm
                remoteFileBaseName, //tar xf
                plan_id,// rm -rf
                flsn, slsn,//cp
                flsn, slsn);//restart
        content = backupCmdString(isFirstTimeFlag) + content+showProcessCmds(flsn,slsn);
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
    private String geneCmdFileForCompleteRelease(String remoteFileFullName, String remoteFileBaseName, boolean isFirstTimeFlag)
    {
       String fmt =
               "if [ -e /msec/%s/%s/ ]; then\n" +
               "cd  /msec/%s/%s/bin/; chmod a+x ./*; ./srpc.sh stop\n"+
               "fi\n" +
               "mkdir -p /msec/%s/%s\n" +
                "rm -rf /msec/%s/%s/*\n"+
               "cp %s /msec/%s/%s/\n" +
               "cd /msec/%s/%s/ ;tar zxf %s\n" +
                "mv /msec/%s/%s/%s/* /msec/%s/%s/\n"+
               "cd /msec/%s/%s/bin/; chmod a+x ./*; ./srpc.sh start\n"+
               "rm /msec/%s/%s/%s";


        String cmdFileName = ServletConfig.fileServerRootDir+"/tmp/"+ plan_id+".sh";
        String content = String.format(fmt,
                flsn, slsn,//if
                flsn, slsn, //stop
                flsn, slsn, //mkdir -p
                flsn, slsn, // rm -rf
                remoteFileFullName, flsn, slsn, //cp
                flsn, slsn, remoteFileBaseName,   //cd ; tar
                flsn, slsn, plan_id, flsn, slsn, //mv
                flsn, slsn,//start
                flsn, slsn, remoteFileBaseName); //rm
        content = backupCmdString(isFirstTimeFlag) + content+showProcessCmds(flsn,slsn);

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

    private long getLastCarryoutTime(DBUtil util) throws Exception
    {
        String sql = "select last_carryout_time from t_release_plan where plan_id=?";
        ArrayList<Object> params = new ArrayList<Object>();
        params.add(plan_id);

        Map<String,Object> result = util.findSimpleResult(sql, params);
        Object o = result.get("last_carryout_time");
        return ((Integer)o).intValue();
    }
    //检查当前时间距离该计划上次发布的时间 是否 大于 gap秒,避免同时执行。 如果大于就写入当前时间并通过检查
    private void checkTime(DBUtil util, long gap, long lastCarryoutTime) throws Exception
    {


        long cur = new Date().getTime() / 1000;

        if (cur < lastCarryoutTime || (cur - lastCarryoutTime ) < gap)
        {
            Exception e = new Exception("太频密，请稍后执行");
            throw  e;
        }
        //更新
        String sql = "update t_release_plan set last_carryout_time=? where plan_id=?";
        ArrayList<Object> params =  new ArrayList<Object>();
        params.add(cur);
        params.add(plan_id);

        util.updateByPreparedStatement(sql,params);

    }
    private void updateReleaseMemo(DBUtil util, IPPortPair ipPortPair) throws Exception
    {
        String sql = "update  t_second_level_service_ipinfo set release_memo=? where ip=? and port=?";
        ArrayList<Object> params = new ArrayList<Object>();
        params.add(release_memo);
        params.add(ipPortPair.getIp());
        params.add(ipPortPair.getPort());

        util.updateByPreparedStatement(sql, params);
    }

    private void doRelease(String tarFile,
                           IPPortPair[] ips,
                           ServletOutputStream out,
                           boolean isFirstTimeFlag,
                           String dev_lang,
                           DBUtil util) throws Exception
    {
        Logger logger = Logger.getLogger(CarryOutReleasePlan.class);

        RemoteShell remoteShell = new RemoteShell();
        String remoteFileBaseName = new File(tarFile).getName();
        String cmdFile = "";
        if (release_type.equals("complete")) {
            cmdFile = geneCmdFileForCompleteRelease("/tmp/" + remoteFileBaseName, remoteFileBaseName, isFirstTimeFlag);
        }
        if (release_type.equals("only_config")) {
            cmdFile = geneCmdFileForOnlyConfigRelease("/tmp/" + remoteFileBaseName, remoteFileBaseName, isFirstTimeFlag);
        }
        if (release_type.equals("only_library")) {
            cmdFile = geneCmdFileForOnlyLibraryRelease("/tmp/" + remoteFileBaseName, remoteFileBaseName, isFirstTimeFlag);
        }
        if (release_type.equals("only_sharedobject")) {

            cmdFile = geneCmdFileForOnlySharedobjectRelease("/tmp/" + remoteFileBaseName, remoteFileBaseName, isFirstTimeFlag, dev_lang);
        }

        logger.info("cmd file:" + cmdFile);


        byte[] buf = new byte[10240];

        for (int i = 0; i < ips.length; i++) {
            String ip = ips[i].getIp();

            String result = remoteShell.SendFileToAgent(tarFile,
                    "/tmp/"+remoteFileBaseName,
                    ip);
            if (result == null || !result.equals("success"))
            {
                safeWrite(String.format(">>>send file to %s failed:%s", ip, result), out);
                continue;
            }
            out.println(String.format("send file to %s succesfully", ip));

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
    String getDevLang(DBUtil util, String flsn, String slsn) throws Exception
    {

        util.getConnection();
        String sql = "select dev_lang from t_second_level_service where first_level_service_name=? and second_level_service_name=?";
        ArrayList<Object> params = new ArrayList<Object>();
        params.add(flsn);
        params.add(slsn);

        Map<String, Object> map = util.findSimpleResult(sql, params);
        return (String) map.get("dev_lang");


    }
    public JsonRPCResponseBase exec(ReleasePlan request)
    {
        Logger logger = Logger.getLogger(CarryOutReleasePlan.class);

        JsonRPCResponseBase resp = new JsonRPCResponseBase();
        plan_id = request.getPlan_id();
        flsn = request.getFirst_level_service_name();
        slsn = request.getSecond_level_service_name();

        logger.info( String.format("carry out plan begin\n [%s.%s][%s]\n", flsn, slsn, plan_id));

        if (flsn == null || flsn.length() < 1 ||
                slsn == null || slsn.length() < 1||
                plan_id == null || plan_id.length() < 1)
        {
            resp.setStatus(100);
            resp.setMessage("service name/plan id invalid!");
            return resp;
        }

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


            String sql = "select dest_ip_list, memo,release_type from t_release_plan where plan_id=?";
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
            release_memo = (String)(res.get("memo"));
            release_type = (String)(res.get("release_type"));
            release_memo = plan_id.substring(4, 12) + release_memo;
            ObjectMapper objectMapper = new ObjectMapper();
            IPPortPair[] ips = objectMapper.readValue(dest_ip_list, IPPortPair[].class);

            logger.info("destination IP number;"+ips.length);

            long gap = ips.length * 60; //每一个目标ip执行耗时估计30s，
            long lastCarryoutTime = getLastCarryoutTime(util);
            checkTime(util,gap, lastCarryoutTime);

            logger.info("check last carry out time OK");




            //对每个IP下发安装包
            String tarFile = PackReleaseFile.getPackedFile(request.getPlan_id());
            logger.info("release package path:" + tarFile);


            getHttpResponse().setContentType("text/html");
            getHttpResponse().setCharacterEncoding("UTF-8");
            ServletOutputStream out = getHttpResponse().getOutputStream();

            safeWrite("<html>", out);
            safeWrite("<head>", out);
            safeWrite("<title>release result</title>", out);
            safeWrite("</head>", out);
            safeWrite("<body>", out);
            safeWrite("<pre>", out);//这个标签用于原样输出

            updatePlanStatus(util, "carrying out");
            String dev_lang = getDevLang(util, flsn,slsn);

            doRelease(tarFile, ips, out, lastCarryoutTime == 0, dev_lang, util);


            safeWrite("</pre>", out);
            safeWrite("</body>", out);
            safeWrite("</html>", out);

            //update plan status
            updatePlanStatus(util, "carry out successfully");


            try {out.close();}catch (Exception e){}
            return null;



        }
        catch (Exception e)
        {
            resp.setStatus(100);
            resp.setMessage(e.getMessage());
            e.printStackTrace();
            try {
                updatePlanStatus(util, "failed to carry out");
            }
            catch (Exception e1){}
            return resp;
        }
        finally {
            util.releaseConn();
        }


    }
}
