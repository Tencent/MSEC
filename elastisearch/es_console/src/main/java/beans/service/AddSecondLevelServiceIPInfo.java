
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

import beans.dbaccess.ServerInfo;
import beans.request.AddSecondLevelServiceIPInfoRequest;
import beans.response.AddSecondLevelServiceIPInfoResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import msec.org.RemoteShell;
import msec.org.Tools;
import org.apache.log4j.Logger;

import java.sql.SQLException;
import java.text.SimpleDateFormat;
import java.util.*;

/**
 *
 * 扩容流程第一步 - 查看服务情况
 */
public class AddSecondLevelServiceIPInfo extends JsonRPCHandler {
    public static String newPlanID()
    {
        double r = Math.random();
        SimpleDateFormat df = new SimpleDateFormat("yyyyMMddHHmmss_");//设置日期格式
        String ret = df.format(new Date());
        ret = ret + (int)(Integer.MAX_VALUE * r);
        return ret;
    }

    public AddSecondLevelServiceIPInfoResponse exec(AddSecondLevelServiceIPInfoRequest request)
    {
        Logger logger = Logger.getLogger("AddSecondLevelServiceIPInfo");
        AddSecondLevelServiceIPInfoResponse response = new AddSecondLevelServiceIPInfoResponse();
        response.setMessage("unknown error.");
        response.setStatus(100);

        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getIp() == null ||
                request.getIp().equals(""))
        {
            response.setMessage("Some request field is empty.");
            response.setStatus(100);
            return response;
        }

        ArrayList<String> ips = Tools.splitBySemicolon(request.getIp());
        Set<String> set_ips = new HashSet<>(ips);

        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setMessage("DB connect failed.");
            response.setStatus(100);
            return response;
        }

        try{
            //check if the IP has been installed in set...
            String sql = "select ip, port, status from t_service_info where first_level_service_name=? and second_level_service_name=? and ip in (";
            List<Object> params = new ArrayList<>();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            for(int i = 0; i < ips.size(); i++) {
                if(i == 0) {
                    params.add(ips.get(i));
                    sql += "?";
                }
                else {
                    params.add(ips.get(i));
                    sql += ",?";
                }
            }
            sql += ")";
            try {
                List<ServerInfo> serverList = util.findMoreRefResult(sql, params, ServerInfo.class);
                if(serverList.size() > 0) {
                    response.setStatus(101);
                    response.setMessage("Input IP(s) is already installed.");
                    return response;
                }
            }
            catch (Exception e)
            {
                response.setStatus(100);
                response.setMessage("db query exception!");
                logger.error(e);
                return response;
            }

            ArrayList<AddSecondLevelServiceIPInfoResponse.IPInfo> addedIPs = new ArrayList<>();
            for(String ip: ips) {
                AddSecondLevelServiceIPInfoResponse.IPInfo info = response.new IPInfo();
                info.setIp(ip);
                RemoteShell remoteShell = new RemoteShell();
                StringBuffer output = new StringBuffer();
                String cmd = "cores=`cat /proc/cpuinfo | grep processor | wc -l`\n"+
                        "mhz=$(printf \"%.0f\" `cat /proc/cpuinfo | grep MHz | head -n1 | awk '{print $4}'`)\n"+
                        "memavail=`grep -i Memtotal /proc/meminfo | awk '{printf(\"%d\", $2*0.8)}'`\n"+
                        "id1=`grep 'cpu ' /proc/stat | awk '{print $5}'`\n"+
                        "all1=`grep 'cpu ' /proc/stat | awk '{print $2+$3+$4+$5+$6+$7}'`\n"+
                        "sleep 1\n"+
                        "id2=`grep 'cpu ' /proc/stat | awk '{print $5}'`\n"+
                        "all2=`grep 'cpu ' /proc/stat | awk '{print $2+$3+$4+$5+$6+$7}'`\n"+
                        "usage=$(printf \"%.0f\" `echo \"scale=1;100-($id2-$id1)*100/($all2-$all1)\"|bc`)\n"+
                        "mkdir -p "+request.getData_dir()+"\n"+
                        "disk_free=`df -Ph "+request.getData_dir()+" | tail -1 | awk '{print $4}'`\n"+
                        "echo -n $cores $mhz $memavail $usage $disk_free";
                result = remoteShell.SendCmdsToRunAndGetResultBack(cmd, ip, output);
                if (result == null || !result.equals("success"))
                {
                    logger.info(String.format("remote error:%s|%s", ip, result));
                    info.setStatus_message("shell");
                }
                else {
                    logger.info(output);
                    String[] ipinfo_results = output.toString().split(" ");
                    if(ipinfo_results.length  != 5)
                    {
                        info.setStatus_message("shell");
                    }
                    else {
                        //core mhz mem cpu free_space
                        info.setCpu_cores(Integer.parseInt(ipinfo_results[0]));
                        info.setCpu_mhz(Integer.parseInt(ipinfo_results[1]));
                        info.setMemory_avail(Integer.parseInt(ipinfo_results[2])/1024);
                        info.setCpu_load(Integer.parseInt(ipinfo_results[3]));
                        info.setData_dir_free_space(ipinfo_results[4]);

                        info.setStatus_message("ok");
                    }
                }
                addedIPs.add(info);

                boolean check_ok = true;
                for(AddSecondLevelServiceIPInfoResponse.IPInfo ip_info: addedIPs) {
                    if(ip_info.getStatus_message().equals("shell")) {
                        check_ok = false;
                    }
                }
                if(!check_ok){
                    response.setMessage("machine");
                }
                else {
                    response.setMessage("success");
                }
                response.setStatus(0);
            }
            response.setAdded_ips(addedIPs);
            return response;
        }
        finally {
            util.releaseConn();
        }
    }
}
