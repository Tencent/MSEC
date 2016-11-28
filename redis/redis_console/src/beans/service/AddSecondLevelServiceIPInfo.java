
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
 * Created by Administrator on 2016/1/27.
 * 扩容流程第一步 - 生成分配方案
 */
public class AddSecondLevelServiceIPInfo extends JsonRPCHandler {

    private String ipinfo_cmd = "cores=`cat /proc/cpuinfo | grep processor | wc -l`\n"+
                "mhz=$(printf \"%.0f\" `cat /proc/cpuinfo | grep MHz | head -n1 | awk '{print $4}'`)\n"+
                "memavail=`grep -i Memtotal /proc/meminfo | awk '{printf(\"%d\", $2*0.8)}'`\n"+
                "id1=`grep 'cpu ' /proc/stat | awk '{print $5}'`\n"+
                "all1=`grep 'cpu ' /proc/stat | awk '{print $2+$3+$4+$5+$6+$7}'`\n"+
                "sleep 1\n"+
                "id2=`grep 'cpu ' /proc/stat | awk '{print $5}'`\n"+
                "all2=`grep 'cpu ' /proc/stat | awk '{print $2+$3+$4+$5+$6+$7}'`\n"+
                "usage=$(printf \"%.0f\" `echo \"scale=1;100-($id2-$id1)*100/($all2-$all1)\"|bc`)\n"+
                "echo -n $cores $mhz $memavail $usage";

    public final static int INSTANCE_MEMORY_STEP = 900;  //实例最小内存

    public void CalcAddedIPForServers(AddSecondLevelServiceIPInfoResponse response, int copy, ArrayList<String> set_ids, int group_id_index, int instance_memory)
    {
        boolean check_ok = true;
        int total_memory = 0;
        Map<String, Integer> set_instance_map = new HashMap<>();
        int i = 0;
        for(AddSecondLevelServiceIPInfoResponse.IPInfo ip : response.getAdded_ips()){
            if(!ip.getStatus_message().equals("shell")) {
                if (ip.getCpu_load() >= 10) {
                    ip.setStatus_message("cpu");
                    check_ok = false;
                } else {
                    if (ip.getInstance_num() < 1) {
                        ip.setStatus_message("memory");
                        check_ok = false;
                    } else {
                        ip.setStatus_message("ok");
                        String set_id = set_ids.get(i/copy);
                        if(set_instance_map.containsKey(set_id))
                            set_instance_map.put(set_id, Math.min(set_instance_map.get(set_id), ip.getInstance_num()));
                        else
                            set_instance_map.put(set_id, ip.getInstance_num());
                        i++;
                    }
                }
            }
            else
                check_ok = false;
        }

        if(!check_ok){
            response.setMessage("machine");
        }
        else {
            response.setMessage("success");
            i = 0;
            int allocated_group_id = group_id_index+1;
            int allocated_instance_num = 0;
            for(AddSecondLevelServiceIPInfoResponse.IPInfo ip : response.getAdded_ips()) {
                ArrayList<ServerInfo> servers = new ArrayList<>();
                String set_id = set_ids.get(i/copy);
                int set_innerid = i % copy;
                ip.setSet_id(set_id);
                int instance_num = set_instance_map.get(set_id);
                for(int j = 0; j <  instance_num; j++) {
                    ServerInfo server = new ServerInfo();
                    server.setGroup_id(j + allocated_group_id);
                    if(j >= allocated_instance_num && j < allocated_instance_num+(instance_num-allocated_instance_num)/(copy-set_innerid)) {
                        server.setMaster(true);
                    }
                    else
                        server.setMaster(false);
                    server.setSet_id(set_id);
                    server.setIp(ip.getIp());
                    server.setPort(10000 + j + allocated_group_id);
                    server.setMemory(instance_memory);
                    servers.add(server);
                }
                ip.setServers(servers);

                allocated_instance_num+=(instance_num-allocated_instance_num)/(copy-set_innerid);
                i++;
                if(set_innerid == copy-1) {
                    allocated_group_id += instance_num;
                    allocated_instance_num = 0;
                }
            }
        }
        response.setStatus(0);
    }

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
        int instance_memory = request.getInstance_type() * INSTANCE_MEMORY_STEP;
        response.setMessage("unkown error.");
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
            response.setMessage("Some request field is  empty.");
            response.setStatus(100);
            return response;
        }

        ArrayList<String> ips = Tools.splitBySemicolon(request.getIp());
        Set<String> set_ips = new HashSet<>(ips);
        if (ips.size() < 2 || set_ips.size() != ips.size())
        {
            response.setMessage("IP input error!");
            response.setStatus(100);
            return response;
        }

        if(ips.size() % request.getCopy() != 0) {
            response.setMessage("the number of IPs is not divisible by the number of copies!");
            response.setStatus(100);
            return response;
        }

        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setMessage("DB connect failed.");
            response.setStatus(100);
            return response;
        }

        try{
            //check if the IP has been installed in set...
            String sql = "select ip, port, group_id, memory, master, status from t_service_info where first_level_service_name=? and second_level_service_name=? and ip in (";
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
                e.printStackTrace();
                return response;
            }

            int group_id_index = 0;
            sql = "select COALESCE(max(group_id),0) as group_id from t_service_info where first_level_service_name=? and second_level_service_name=?";
            params.clear();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            try {
                Map<String, Object> result_map = util.findSimpleResult(sql, params);
                if(result_map.size() != 1) {
                    response.setStatus(101);
                    response.setMessage("db set error!");
                    return response;
                }
                group_id_index = ((Long)result_map.get("group_id")).intValue();
            }
            catch (Exception e)
            {
                response.setStatus(100);
                response.setMessage("db query exception!");
                e.printStackTrace();
                return response;
            }

            ArrayList<AddSecondLevelServiceIPInfoResponse.IPInfo> addedIPs = new ArrayList<>();


            for(String ip: ips) {
                AddSecondLevelServiceIPInfoResponse.IPInfo info = response.new IPInfo();
                info.setIp(ip);
                RemoteShell remoteShell = new RemoteShell();
                StringBuffer output = new StringBuffer();
                result = remoteShell.SendCmdsToRunAndGetResultBack(ipinfo_cmd, ip, output);
                if (result == null || !result.equals("success"))
                {
                    logger.info(String.format("remote error:%s|%s", ip, result));
                    info.setStatus_message("shell");
                }
                else {
                    logger.info(output);
                    String[] ipinfo_results = output.toString().split(" ");
                    if(ipinfo_results.length  != 4)
                    {
                        info.setStatus_message("shell");
                    }
                    else {
                        //core mhz mem cpu
                        info.setCpu_cores(Integer.parseInt(ipinfo_results[0]));
                        info.setCpu_mhz(Integer.parseInt(ipinfo_results[1]));
                        info.setMemory_avail(Integer.parseInt(ipinfo_results[2])/1024);
                        info.setCpu_load(Integer.parseInt(ipinfo_results[3]));

                    }
                }
                addedIPs.add(info);
            }

            response.setAdded_ips(addedIPs, instance_memory);
            ArrayList<String> set_ids = new ArrayList<>();

            //get set ids
            int set_num = ips.size() / request.getCopy();
            sql = "select name from t_set_id_name where name not in (select distinct set_id from t_service_info where first_level_service_name=? and second_level_service_name=?) order by name asc limit ?";
            params.clear();
            params.add(request.getFirst_level_service_name());
            params.add(request.getSecond_level_service_name());
            params.add(set_num);
            try {
                ArrayList<Map<String, Object>> result_list = util.findModeResult(sql, params);
                if(result_list.size() != set_num) {
                    response.setStatus(101);
                    response.setMessage("db set error!");
                    return response;
                }
                for(int i = 0; i < result_list.size(); i++)
                    set_ids.add(result_list.get(i).get("name").toString());
            } catch (SQLException e) {
                response.setStatus(100);
                response.setMessage("db query exception!");
                e.printStackTrace();
                return response;
            }

            CalcAddedIPForServers(response, request.getCopy(), set_ids, group_id_index, instance_memory);

            if(response.getMessage().equals("success") && group_id_index == 0) {
                //update t_second_level_service
                logger.info(String.format("Create|Update copy_num & mem_per_instance|%d|%d", group_id_index, instance_memory));
                sql = "update t_second_level_service set copy_num=?, memory_per_instance=? where first_level_service_name=? and second_level_service_name=?";
                params.clear();
                params.add(request.getCopy());
                params.add(instance_memory);
                params.add(request.getFirst_level_service_name());
                params.add(request.getSecond_level_service_name());
                try {
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum < 0) {
                        response.setMessage("failed to update table");
                        response.setStatus(100);
                        return response;
                    }
                } catch (SQLException e) {
                    response.setMessage("update status failed:" + e.toString());
                    response.setStatus(100);
                    e.printStackTrace();
                    return response;
                }
            }
            else
                logger.info(group_id_index);



            return response;
        }
        finally {
            util.releaseConn();
        }
    }
}
