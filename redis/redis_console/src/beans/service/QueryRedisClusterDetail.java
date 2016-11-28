
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

import beans.request.QueryRedisClusterDetailRequest;
import beans.response.QueryRedisClusterDetailResponse;
import beans.response.QuerySecondLevelServiceDetailResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;
import redis.clients.jedis.Jedis;
import redis.clients.jedis.exceptions.JedisConnectionException;
import redis.clients.jedis.exceptions.JedisDataException;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class QueryRedisClusterDetail extends JsonRPCHandler {
    DBUtil util;
    HashMap<String,Integer> master_nodes;
    HashMap<String,Integer> slave_nodes;

    private void getStatus(String first_level_service_name, String second_level_service_name) throws  SQLException {
        Logger logger = Logger.getLogger(QueryRedisClusterDetail.class);
        String sql = "select ip, port, group_id, master from t_service_info where first_level_service_name=? and second_level_service_name=?";
        List<Object> params = new ArrayList<Object>();
        params.add(first_level_service_name);
        params.add(second_level_service_name);

        ArrayList<Map<String, Object>> result_list = util.findModeResult(sql, params);

        for(int i = 0; i < result_list.size(); i++){
            if(Boolean.parseBoolean(result_list.get(i).get("master").toString())) {
                master_nodes.put(result_list.get(i).get("ip").toString() + ":" + result_list.get(i).get("port").toString(), Integer.parseInt(result_list.get(i).get("group_id").toString()));

            }
            else
                slave_nodes.put(result_list.get(i).get("ip").toString() + ":" + result_list.get(i).get("port").toString(), Integer.parseInt(result_list.get(i).get("group_id").toString()));
        }

        logger.info(master_nodes);
        logger.info(slave_nodes);
    }

    private void updateStatus(String first_level_service_name, String second_level_service_name, String ip, int port, QueryRedisClusterDetailResponse.RTInfo info) throws SQLException
    {
        Logger logger = Logger.getLogger(QueryRedisClusterDetail.class);

        String sql = "";
        List<Object> params = new ArrayList<Object>();
        if(info.isOK()) {
            sql = "update t_service_info set master=?,status=? where first_level_service_name=? and second_level_service_name=? and ip=? and port=?";
            params.add(info.isMaster());
            params.add(info.getStatus());
        }
        else
        {
            sql = "update t_service_info set status=? where first_level_service_name=? and second_level_service_name=? and ip=? and port=?";
            params.add(info.getStatus());
        }

        params.add(first_level_service_name);
        params.add(second_level_service_name);
        params.add(ip);
        params.add(port);

        int updNum = util.updateByPreparedStatement(sql, params);
        if (updNum != 1) {
            logger.error(String.format("updateStatus|%d", updNum));
        }
    }

    private boolean updatePlan(String first_level_service_name, String second_level_service_name) throws SQLException {
        Logger logger = Logger.getLogger(QueryRedisClusterDetailResponse.class);
        String sql = "select distinct status from t_install_plan where plan_id=(select plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?)";
        List<Object> params = new ArrayList<Object>();
        params.add(first_level_service_name);
        params.add(second_level_service_name);
        ArrayList<Map<String, Object>> status_infos = util.findModeResult(sql, params);
        if(status_infos.size() == 1 && status_infos.get(0).get("status").toString().startsWith("Done")) { //check if it is "Done"
            sql = "update t_second_level_service set plan_id=\"\" where first_level_service_name=? and second_level_service_name=?";
            int addNum = util.updateByPreparedStatement(sql, params);
            if (addNum < 0) {
                throw new SQLException("Done update plan_id");
            }
            logger.info("updatePlan_done");
            return true;
        } else {
            for(int i = 0; i < status_infos.size(); i++) {
                if (status_infos.get(i).get("status").toString().startsWith("[ERROR]")) {
                    sql = "update t_second_level_service set plan_id=\"\" where first_level_service_name=? and second_level_service_name=?";
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum < 0) {
                        throw new SQLException("ERROR update plan_id");
                    }
                    logger.info("updatePlan_ERROR");
                    return true;
                }
            }
        }
        return  false;
    }

    private void updateMaster(String first_level_service_name, String second_level_service_name, String ip, int port, int group_id) throws SQLException
    {
        Logger logger = Logger.getLogger(QueryRedisClusterDetailResponse.class);

        String sql = "update t_service_info set master= case when ip=? and port=? then 1 else 0 end where first_level_service_name=? and second_level_service_name=? and group_id=?";
        List<Object> params = new ArrayList<Object>();
        params.add(ip);
        params.add(port);
        params.add(first_level_service_name);
        params.add(second_level_service_name);
        params.add(group_id);

        int updNum = util.updateByPreparedStatement(sql, params);
        if(updNum == 0)
            logger.error(String.format("updateMaster|%d", updNum));
    }

    public boolean checkMaster(Jedis jedis) {
        for (String infoLine : jedis.clusterNodes().split("\n")) {
            if (infoLine.contains("myself")) {
                if(infoLine.contains("master"))
                    return true;
                else
                    return false;
            }
        }
        return false;
    }

    public QueryRedisClusterDetailResponse exec(QueryRedisClusterDetailRequest request)
    {
        Logger logger = Logger.getLogger(QueryRedisClusterDetail.class);
        QueryRedisClusterDetailResponse resp = new QueryRedisClusterDetailResponse();

        String result = checkIdentity();
        if (!result.equals("success"))
        {
            resp.setStatus(99);
            resp.setMessage(result);
            return resp;
        }

        boolean all_ok = true;
        int master_num = 0;

        util = new DBUtil();
        if (util.getConnection() == null)
        {
            resp.setStatus(100);
            resp.setMessage("db connect failed!");
            return resp;
        }


        master_nodes = new HashMap<>();
        slave_nodes = new HashMap<>();

        HashMap<String,Integer> master_ips = new HashMap<>();
        HashMap<String, QueryRedisClusterDetailResponse.RTInfo> host_infos = new HashMap<>();
        HashMap<String, Long> host_seqs = new HashMap<>();
        ArrayList<String> masters_with_no_slaves = new ArrayList<>();
        HashMap<String, QueryRedisClusterDetailResponse.RTInfo> info_map = new HashMap<>();

        try {
            if(updatePlan(request.getFirst_level_service_name(), request.getSecond_level_service_name())) {
                resp.setMessage("refresh");
                resp.setStatus(101);
                return resp;
            }

            getStatus(request.getFirst_level_service_name(), request.getSecond_level_service_name());

            for (String host : request.getHosts()) {
                Jedis jedis = null;
                String[] ip_pair = host.split(":");
                QueryRedisClusterDetailResponse.RTInfo rt_info = resp.new RTInfo();
                try {
                    jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                    String info = "";
                    try{
                        info = jedis.clusterInfo();
                    }
                    catch (JedisDataException ex) {
                        if(ex.getMessage().startsWith("LOADING")) {
                            rt_info.setStatus("Syncing");
                        }
                        else {
                            rt_info.setStatus(ex.getMessage());
                        }
                        host_infos.put(host, rt_info);
                        continue;
                    }
                    if (!jedis.clusterInfo().split("\n")[0].contains("ok")) {
                        rt_info.setStatus("Cluster state error");
                    } else {
                        rt_info.setStatus("OK");
                        rt_info.setOK(true);
                        rt_info.setMaster(checkMaster(jedis));
                        //if it is master..
                        if(rt_info.isMaster()) {
                            //add master_ips
                            int count = master_ips.containsKey(ip_pair[0]) ? master_ips.get(ip_pair[0]) : 0;
                            master_ips.put(ip_pair[0], count + 1);
                            master_num+=1;
                            if(slave_nodes.containsKey(host)) {
                                updateMaster(request.getFirst_level_service_name(), request.getSecond_level_service_name(), ip_pair[0], Integer.parseInt(ip_pair[1]), slave_nodes.get(host));
                                resp.setMessage("refresh");
                                resp.setStatus(101);
                                return resp;
                            }

                            //get data seq
                            String[] infos = jedis.info("replication").split("\r\n");
                            boolean has_slave = false;
                            HashMap<String, Long> hs = new HashMap<>();
                            long master_seq = -1;
                            for(String line: infos) {
                                if(line.startsWith("slave")){
                                    has_slave = true;
                                    String[] cols = line.split(",");
                                    if(cols.length >= 4) {
                                        String ip = cols[0].split("=")[1];
                                        String port = cols[1].split("=")[1];
                                        long seq = Long.parseLong(cols[3].split("=")[1]);
                                        hs.put(ip+":"+port, seq);
                                    }
                                }
                                else if (line.startsWith("master_repl_offset")) {
                                    master_seq = Long.parseLong(line.split(":")[1]);
                                }
                            }
                            if(master_seq >= 0) {
                                for(Map.Entry<String, Long> entry : hs.entrySet()) {
                                    host_seqs.put(entry.getKey(), master_seq -  entry.getValue());
                                }
                            }
                            if(!has_slave) {
                                masters_with_no_slaves.add(host);
                            }
                        }
                    }
                    host_infos.put(host, rt_info);
                } catch (JedisConnectionException e) {
                    rt_info.setStatus("Connection lost");
                    host_infos.put(host, rt_info);
                } finally {
                    if (jedis != null)
                        jedis.close();
                }
            }

            for (String host : request.getHosts()) {
                String[] ip_pair = host.split(":");
                QueryRedisClusterDetailResponse.RTInfo rt_info = host_infos.get(host);
                if(rt_info != null) {
                    if(host_seqs.containsKey(host))
                        rt_info.setSeq(host_seqs.get(host));
                    if(!rt_info.isOK()) {
                        logger.info(String.format("host failed|%s", host));
                        all_ok = false;
                    }
                    info_map.put(host, rt_info);
                    updateStatus(request.getFirst_level_service_name(), request.getSecond_level_service_name(), ip_pair[0], Integer.parseInt(ip_pair[1]), rt_info);
                }
            }
        }
        catch (Exception e) {
            e.printStackTrace();
            logger.error(e.getMessage());
        } finally {
            util.releaseConn();
        }
        resp.setInfo_map(info_map);
        if(all_ok) {
            for (Map.Entry<String, Integer> entry : master_ips.entrySet()) {
                if (entry.getValue() * 2 >= master_num) {
                    resp.setWarning(String.format("超过一半的主机部署在%s上，如果该IP出现问题，redis cluster将不能自动恢复。请尽快对该IP进行主从切换或对cluster进行扩容。", entry.getKey()));
                    break;
                }
            }
        }
        else
        {
            if(masters_with_no_slaves.size() > 0) {
                String message = "下述主机只有一份拷贝，请尽快添加拷贝：";
                for(String host : masters_with_no_slaves) {
                    message+="<br/>    "+host;
                }
                resp.setWarning(message);
            }
        }
        resp.setMessage("success");
        resp.setStatus(0);

        return resp;
    }
}
