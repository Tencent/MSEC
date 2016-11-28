
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
import beans.dbaccess.ClusterInfo;
import beans.dbaccess.ServerInfo;
import msec.org.DBUtil;
import org.apache.log4j.Logger;
import redis.clients.jedis.*;
import redis.clients.jedis.exceptions.JedisClusterException;
import redis.clients.jedis.exceptions.JedisConnectionException;
import redis.clients.jedis.exceptions.JedisDataException;

import java.lang.reflect.Array;
import java.util.*;
/**
 * Created on 2016/6/1.
 */
public class JedisHelper {
    public class JedisHelperException extends RuntimeException {
        public JedisHelperException(String message) {
            super(message);
        }

        public JedisHelperException(Throwable e) {
            super(e);
        }

        public JedisHelperException(String message, Throwable cause) {
            super(message, cause);
        }
    }

    public class ClusterStatus {
        public String getNodeid() {
            return nodeid;
        }

        public void setNodeid(String nodeid) {
            this.nodeid = nodeid;
        }

        public String getIp_port() {
            return ip_port;
        }

        public void setIp_port(String ip_port) {
            this.ip_port = ip_port;
        }

        public String getError_message() {
            return error_message;
        }

        public void setError_message(String error_message) {
            this.error_message = error_message;
        }

        public boolean isMaster() {
            return master;
        }

        public void setMaster(boolean master) {
            this.master = master;
        }

        public String getMaster_nodeid() {
            return master_nodeid;
        }

        public void setMaster_nodeid(String master_nodeid) {
            this.master_nodeid = master_nodeid;
        }

        public String getMaster_ip() {
            return master_ip;
        }

        public void setMaster_ip(String master_ip) {
            this.master_ip = master_ip;
        }

        public TreeSet<Integer> getRunning_slots() {
            return running_slots;
        }

        public void setRunning_slots(TreeSet<Integer> running_slots) {
            this.running_slots = running_slots;
        }

        public TreeSet<Integer> getImporting_slots() {
            return importing_slots;
        }

        public void setImporting_slots(TreeSet<Integer> importing_slots) {
            this.importing_slots = importing_slots;
        }

        public TreeSet<Integer> getMigrating_slots() {
            return migrating_slots;
        }

        public void setMigrating_slots(TreeSet<Integer> migrating_slots) {
            this.migrating_slots = migrating_slots;
        }

        public boolean isOK() {
            return OK;
        }

        public void setOK(boolean OK) {
            this.OK = OK;
        }

        public boolean isEmpty() {
            return empty;
        }

        public void setEmpty(boolean empty) {
            this.empty = empty;
        }

        String nodeid;
        String ip_port; //ip:port
        String error_message;   //error message
        boolean master;
        String master_nodeid;   //master nodeid
        String master_ip;        //master ip
        TreeSet<Integer> running_slots; //without importing and migrating slots
        TreeSet<Integer> importing_slots;
        TreeSet<Integer> migrating_slots;
        boolean OK;    //state
        boolean empty;  //empty server
    }
    public class ShardingPlanInfo {
        public ShardingPlanInfo() {
            slot_id = new ArrayList<>();
        }

        public String getFrom_ip() {
            return from_ip;
        }

        public void setFrom_ip(String from_ip) {
            this.from_ip = from_ip;
        }

        public String getTo_ip() {
            return to_ip;
        }

        public void setTo_ip(String to_ip) {
            this.to_ip = to_ip;
        }

        public ArrayList<Integer> getSlot_id() {
            return slot_id;
        }

        public void setSlot_id(ArrayList<Integer> slot_id) {
            this.slot_id = slot_id;
        }

        String from_ip;
        String to_ip;
        ArrayList<Integer> slot_id;
    }

    public static String join(Collection<?> col, String delim) {
        StringBuilder sb = new StringBuilder();
        Iterator<?> iter = col.iterator();
        if (iter.hasNext())
            sb.append(iter.next().toString());
        while (iter.hasNext()) {
            sb.append(delim);
            sb.append(iter.next().toString());
        }
        return sb.toString();
    }

    public static String join(Map<?,?> map, String delim) {
        StringBuilder sb = new StringBuilder();
        Iterator<?> iter = map.entrySet().iterator();
        if (iter.hasNext()) {
            Map.Entry<?,?> entry = (Map.Entry<?,?>)iter.next();
            sb.append(entry.getKey() + ":" + entry.getValue());
        }
        while (iter.hasNext()) {
            sb.append(delim);
            Map.Entry<?,?> entry = (Map.Entry<?,?>)iter.next();
            sb.append(entry.getKey()+":"+ entry.getValue());
        }
        return sb.toString();
    }

    public JedisHelper(String plan_id_, String ip_, int port_, int copy_) {
        plan_id = plan_id_;
        ip = ip_;
        port = port_;
        copy = copy_;
        cluster = new HashMap<>();
        master_nodes = new HashMap<>();
        fail_nodes = new ArrayList<>();
        allocated_slots = new TreeSet<>();
        cluster_status_map = new HashMap<>();
        master_acks = new HashMap<>();
        slot_sigs = new HashMap<>();
        master_slave_infos = new HashMap<>();

        Jedis jedis = new Jedis(ip, port);
        jedis.connect();
        cluster.put(ip_ + ":" + port_, jedis);
    }

    public JedisHelper(HashMap<String, ServerInfo> host_map_, String first_level_service_name_, String second_level_service_name_, ClusterInfo cluster_info_) {
        host_map = host_map_;
        first_level_service_name = first_level_service_name_;
        second_level_service_name = second_level_service_name_;
        cluster_info = cluster_info_;
        changed = false;
        cluster = new HashMap<>();
        master_nodes = new HashMap<>();
        fail_nodes = new ArrayList<>();
        allocated_slots = new TreeSet<>();
        cluster_status_map = new HashMap<>();
        master_acks = new HashMap<>();
        slot_sigs = new HashMap<>();
        master_slave_infos = new HashMap<>();
        for(String host: host_map.keySet()) {
            String[] ip_pair = host.split(":");
            Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
            try {
                jedis.connect();
                cluster.put(host, jedis);
            }
            catch(JedisConnectionException e) {
                cluster.put(host, null);
            }
        }
    }

    public void CheckOneServer(String host, Jedis jedis) {
        Logger logger = Logger.getLogger(JedisHelper.class);
        ClusterStatus status = new ClusterStatus();
        if(jedis == null) {
            status.setIp_port(host);
            status.setOK(false);
            status.setError_message("Connection lost");
            cluster_status_map.put(host, status);
            return;
        }
        try {
            String str_nodes = "";
            try {
                str_nodes = jedis.clusterNodes();
            }
            catch(JedisDataException ex) {
                status.setIp_port(host);
                status.setOK(false);
                status.setError_message(ex.toString());
                cluster_status_map.put(host, status);
                logger.error(String.format("Exception|%s|", host), ex);
                return;
            }
            TreeMap<String, String> config_map = new TreeMap<>();   //node_id - [slots]
            String master_nodeid = "";
            String master_ip = "";
            HashMap<String, ArrayList<String>> master_slaves = new HashMap<>();
            for (String node_info : str_nodes.split("\n")) {
                if (node_info.contains("myself")) {
                    String[] node_status = node_info.split(" ");
                    if (node_status.length < 8)  //slave = 8, master = 9+
                    {
                        status.setIp_port(host);
                        status.setOK(false);
                        cluster_status_map.put(host, status);
                        continue;
                    } else {
                        status.setNodeid(node_status[0]);
                        status.setIp_port(node_status[1]);
                        if (node_status[2].contains("master")) {
                            master_nodeid = node_status[0];
                            master_ip = node_status[1];
                            ArrayList<String> slots = new ArrayList<>();
                            status.setMaster(true);
                            status.setOK(true);
                            //check slot
                            TreeSet<Integer> running_slots = new TreeSet<>();
                            TreeSet<Integer> importing_slots = new TreeSet<>();
                            TreeSet<Integer> migrating_slots = new TreeSet<>();
                            for (int i = 8; i < node_status.length; i++) {
                                if (node_status[i].startsWith("[")) {  //importing/migrating slot
                                    int idx =  node_status[i].indexOf("-<-");
                                    if(idx > 0) {
                                        importing_slots.add(Integer.parseInt(node_status[i].substring(1, idx)));
                                    }
                                    idx = node_status[i].indexOf("->-");
                                    if(idx > 0) {
                                        migrating_slots.add(Integer.parseInt(node_status[i].substring(1, idx)));
                                    }
                                    setMigrating(true);
                                } else {
                                    slots.add(node_status[i]);
                                    int idx = node_status[i].indexOf("-");
                                    if (idx > 0) {
                                        int start = Integer.parseInt(node_status[i].substring(0, idx));
                                        int end = Integer.parseInt(node_status[i].substring(idx + 1));
                                        for (int j = start; j <= end; j++) {
                                            running_slots.add(j);
                                        }
                                    } else {
                                        running_slots.add(new Integer(node_status[i]));
                                    }
                                    Collections.sort(slots);
                                    config_map.put(node_status[0], join(slots, ","));
                                }
                            }
                            status.setRunning_slots(running_slots);
                            status.setImporting_slots(importing_slots);
                            status.setMigrating_slots(migrating_slots);
                            master_nodes.put(host, running_slots);
                            logger.info(String.format("%s|%d", host, running_slots.size()));
                            allocated_slots.addAll(running_slots);
                        } else if (node_status[2].contains("slave")) {
                            status.setMaster(false);
                            status.setOK(true);
                            status.setMaster_nodeid(node_status[3]);
                            if(master_slaves.get(node_status[3]) == null){
                                master_slaves.put(node_status[3], new ArrayList<String>());
                            }
                            master_slaves.get(node_status[3]).add(node_status[1]);
                        } else {
                            status.setOK(false);
                            logger.error(str_nodes);
                        }
                        cluster_status_map.put(host, status);
                        //nodeid_status_map.put(status.getNodeid(), status);
                    }
                }
                else if (node_info.contains("master")) {
                    String[] node_status = node_info.split(" ");
                    ArrayList<String> slots = new ArrayList<>();
                    for (int i = 8; i < node_status.length; i++) {
                        if (!node_status[i].startsWith("[")) {
                            slots.add(node_status[i]);
                        }
                    }
                    if(slots.size() > 0) {
                        Collections.sort(slots);
                        config_map.put(node_status[0], join(slots,","));
                    }

                    if(master_acks.get(node_status[1]) == null) {
                        master_acks.put(node_status[1], new ArrayList<String>());
                    }
                    master_acks.get(node_status[1]).add(host);
                }
                else if (node_info.contains("slave")) {
                    if (node_info.contains("fail") && !node_info.contains("fail?")) {//fail
                        logger.info(String.format("OmitFail|%s", node_info));
                    }
                    else {
                        String[] node_status = node_info.split(" ");
                        status.setMaster_nodeid(node_status[3]);
                        if (master_slaves.get(node_status[3]) == null) {
                            master_slaves.put(node_status[3], new ArrayList<String>());
                        }
                        master_slaves.get(node_status[3]).add(node_status[1]);
                    }
                }
            }

            if(config_map.size() > 1) { //only for multiple masters...
                String config_signature = join(config_map, "|");
                slot_sigs.put(host, config_signature);
            }

            if(!master_nodeid.isEmpty()) {  //master
                ArrayList<String> slaves = master_slaves.get(master_nodeid);
                if(slaves != null)
                    master_slave_infos.put(master_ip, slaves);
            }

        } catch (JedisConnectionException e) {
            logger.error(String.format("Exception|%s|", host), e);
            status.setError_message(e.toString());
            status.setIp_port(host);
            status.setOK(false);
            cluster_status_map.put(host, status);
        }
    }

    public HashMap<String, ClusterStatus> CheckStatusDetail() {
        Logger logger = Logger.getLogger(JedisHelper.class);
        //HashMap<String, ClusterStatus> nodeid_status_map = new HashMap<>();
        setMigrating(false);
        setCreated(false);
        setOK(true);
        master_nodes.clear();
        allocated_slots.clear();
        cluster_status_map.clear();
        master_acks.clear();
        slot_sigs.clear();
        master_slave_infos.clear();
        for (Map.Entry<String, Jedis> entry : cluster.entrySet()) {
            //get cluster nodes...
            //<id> <ip:port> <flags> <master> <ping-sent> <pong-recv> <config-epoch> <link-state> <slot> <slot> ... <slot>
            //we only check myself node to get detailed info...
            //If jedis is null, we omit and discover this node from other live masters....
            CheckOneServer(entry.getKey(), entry.getValue());
        }

        //check config consistence, alarm!
        if(slot_sigs.size() > 0) {
            HashSet<String> hs = new HashSet<>();
            hs.addAll(slot_sigs.values());
            if(hs.size() != 1) {
                setOK(false);
                setError_message("[ERROR] Nodes don't agree about config! Please consult DBA for help.");
                logger.error(getError_message());
                logger.error(slot_sigs);
                return cluster_status_map;
            }
        }


        //check new ips...
        for(Map.Entry<String, ArrayList<String>> entry :  master_acks.entrySet()) {
            if(!host_map.containsKey(entry.getKey())) {   //check new master
                logger.info(String.format("AddMaster|%s", entry.getKey()));
                try {
                    String[] ip_pair = entry.getKey().split(":");
                    Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                    jedis.connect();
                    cluster.put(entry.getKey(), jedis);
                    CheckOneServer(entry.getKey(), jedis);
                    insertInfo(ip_pair[0], Integer.parseInt(ip_pair[1]), "");
                }
                catch(JedisConnectionException ex) {
                    cluster.put(entry.getKey(), null);
                }
            }
            else {
                if(!host_map.get(entry.getKey()).isMaster()) {
                    //TODO,update to master
                }
            }
            ArrayList<String> slaves = master_slave_infos.get(entry.getKey());
            if(slaves != null) {
                for (String slave : slaves) { //check new slaves
                    if (!host_map.containsKey(slave)) {
                        logger.info(String.format("AddSlave|%s|%s", slave, entry.getKey()));
                        String[] ip_pair = slave.split(":");
                        Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                        jedis.connect();
                        cluster.put(slave, jedis);
                        CheckOneServer(slave, jedis);
                        insertInfo(ip_pair[0], Integer.parseInt(ip_pair[1]), entry.getKey());
                    } else {
                        if (host_map.get(slave).getSet_id() != host_map.get(entry.getKey()).getSet_id()) {
                            //TODO, update to other slave
                        }
                    }
                }
            }
        }

        logger.info(master_acks);
        logger.info(master_slave_infos);
        logger.info(host_map);

        //check slot coverage, alarm!
        if(allocated_slots.size() != CLUSTER_HASH_SLOTS) {
            setOK(false);
            setError_message("[ERROR] Nodes don't agree about config! Please consult DBA for help.");
            logger.error(getError_message());
            logger.error(allocated_slots.size());
            return cluster_status_map;
        }
        else
            setCreated(true);

        //check open slot, fixable -> migrating
        //remove invalid set...
        ArrayList<String> invalid_set_ids = new ArrayList<>();
        for(Map.Entry<String, ServerInfo> entry : host_map.entrySet()) {
            if(entry.getValue().isMaster() && master_acks.get(entry.getKey()) == null)  {
                invalid_set_ids.add(entry.getValue().getSet_id());
            }
        }
        if(invalid_set_ids.size() > 0) {
            logger.info(String.format("deleteInfo|%s", invalid_set_ids));
        }

        for(String set_id : invalid_set_ids)
            deleteInfo(set_id);
/*
        for(Map.Entry<String, ClusterStatus> entry : cluster_status_map.entrySet()) {
            ClusterStatus status = entry.getValue();
            String s;
            if(status.isOK()) {
                if (status.isMaster())
                    s = "M ";
                else
                    s = "S ";
                s += status.getIp_port();
                if (!status.isMaster()) {
                    ClusterStatus master_status = nodeid_status_map.get(status.getMaster_nodeid());
                    if(master_status != null)
                    {
                        s += " " + master_status.getIp_port();
                    }
                    else
                    {
                        s += " master error";
                    }
                }
                else {   //slave don't have slots...
                    s += " " + Integer.toString(status.getRunning_slots().size());
                }
            }
            else
            {
                s = "FAIL " + entry.getKey();
            }
            logger.info(s);
        }
        */
        return cluster_status_map;
    }

    private void updateStatus(String ip, int port, String status)
    {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "update t_install_plan set status=? where plan_id=? and ip=? and port=?";
            List<Object> params = new ArrayList<Object>();
            params.add(status);
            params.add(plan_id);
            params.add(ip);
            params.add(port);

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum != 1) {
                return;
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error(e.getMessage());
            return;
        } finally {
            util.releaseConn();
        }
    }

    private void updateStatus(String status)
    {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "update t_install_plan set status=? where plan_id=?";
            List<Object> params = new ArrayList<Object>();
            params.add(status);
            params.add(plan_id);

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum < 0) {
                return;
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error(e.getMessage());
            return;
        } finally {
            util.releaseConn();
        }
    }

    private void insertInfo(String ip, int port, String master_host)
    {
        changed = true;
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            if(master_host.isEmpty()) {
                String sql = "select name from t_set_id_name where name not in (select distinct set_id from t_service_info where first_level_service_name=? and second_level_service_name=?) order by name asc limit 1";
                List<Object> params = new ArrayList<Object>();
                params.add(first_level_service_name);
                params.add(second_level_service_name);

                Map<String, Object> result = util.findSimpleResult(sql, params);
                String set_id = result.get("name").toString();

                sql = "select COALESCE(max(group_id),0) as group_id from t_service_info where first_level_service_name=? and second_level_service_name=?";
                result = util.findSimpleResult(sql, params);
                if (result.size() != 1) {
                    logger.error("get_group_id ERROR");
                    return;
                }
                int group_id = ((Long) result.get("group_id")).intValue() + 1;

                ServerInfo server_info = new ServerInfo();
                server_info.setIp(ip);
                server_info.setPort(port);
                server_info.setSet_id(set_id);
                server_info.setGroup_id(group_id);
                server_info.setMemory(cluster_info.getMemory_per_instance());
                server_info.setMaster(true);

                sql = "insert into t_service_info(first_level_service_name, second_level_service_name, ip, port, set_id, group_id, memory, master) values(?,?,?,?,?,?,?,?)";
                params.add(ip);
                params.add(port);
                params.add(set_id);
                params.add(group_id);
                params.add(cluster_info.getMemory_per_instance());
                params.add(true);

                int updNum = util.updateByPreparedStatement(sql, params);
                if (updNum < 0) {
                    logger.error(String.format("insert_master ERROR|%s|%d", ip, port));
                    return;
                }
                host_map.put(ip+":"+port, server_info);
            } else {
                ServerInfo server_info = host_map.get(master_host);
                if(server_info == null) {
                    logger.error(String.format("insert_master ERROR|Master %s not found", master_host));
                }
                else {
                    ServerInfo slave_info = new ServerInfo(server_info);
                    slave_info.setIp(ip);
                    slave_info.setPort(port);
                    slave_info.setMaster(false);
                    String sql = "insert into t_service_info(first_level_service_name, second_level_service_name, ip, port, set_id, group_id, memory, master) values(?,?,?,?,?,?,?,?)";
                    List<Object> params = new ArrayList<Object>();
                    params.add(first_level_service_name);
                    params.add(second_level_service_name);
                    params.add(ip);
                    params.add(port);
                    params.add(slave_info.getSet_id());
                    params.add(slave_info.getGroup_id());
                    params.add(cluster_info.getMemory_per_instance());
                    params.add(false);
                    int updNum = util.updateByPreparedStatement(sql, params);
                    if (updNum < 0) {
                        logger.error(String.format("insert_slave ERROR|%s|%d|%s", ip, port, master_host));
                        return;
                    }
                    host_map.put(ip + ":" + port, slave_info);
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error(e.getMessage());
            return;
        } finally {
            util.releaseConn();
        }
    }

    private void deleteInfo(String set_id)
    {
        changed = true;
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "delete from t_service_info where first_level_service_name=? and second_level_service_name=? and set_id=?";
            List<Object> params = new ArrayList<Object>();
            params.add(first_level_service_name);
            params.add(second_level_service_name);
            params.add(set_id);

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum < 0) {
                logger.error(String.format("delete_set ERROR|%s", set_id));
                return;
            }

            for(Iterator<Map.Entry<String, ServerInfo>> it = host_map.entrySet().iterator(); it.hasNext(); ) {
                Map.Entry<String, ServerInfo> entry = it.next();
                if(entry.getValue().getSet_id().equals(set_id)) {
                    it.remove();
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
            logger.error(e.getMessage());
            return;
        } finally {
            util.releaseConn();
        }
    }

    public HashMap<String, ClusterStatus> CheckStatus() {
        Logger logger = Logger.getLogger(JedisHelper.class);
        HashMap<String, ClusterStatus> nodeid_status_map = new HashMap<>();
        setMigrating(false);
        setCreated(false);
        setOK(true);
        master_nodes.clear();
        allocated_slots.clear();
        cluster_status_map.clear();

        if(cluster.size() ==1) {
            Map.Entry<String, Jedis> entry = cluster.entrySet().iterator().next();
            String str_nodes = entry.getValue().clusterNodes();
            logger.info(str_nodes);
            for (String node_info : str_nodes.split("\n")) {
                //<id> <ip:pmort> <flags> <master> <ping-sent> <pong-recv> <config-epoch> <link-state> <slot> <slot> ... <slot>
                //we only check myself node to get detailed info...
                if(node_info.contains("myself"))
                    continue;
                String[] node_status = node_info.split(" ");
                if (node_status.length < 8)  //slave = 8, master = 9+
                {
                    throw new JedisHelperException(String.format("[ERROR] Node|%s", node_info));
                } else {
                    if (!cluster.containsKey(node_status[1])) {
                        if (node_info.contains("fail") && !node_info.contains("fail?")) {//fail
                            fail_nodes.add(node_status[0]);
                        }
                        else {
                            String[] ip_pair = node_status[1].split(":");
                            try {
                                Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                                jedis.connect();
                                cluster.put(node_status[1], jedis);
                            }
                            catch(JedisConnectionException ex) {
                                if(node_info.contains("fail"))  //fail? -> fail
                                    fail_nodes.add(node_status[0]);
                                else
                                    throw new JedisHelperException(String.format("[ERROR] Node|%s", node_info));
                            }
                        }
                    }
                }
            }
        }
        for (Map.Entry<String, Jedis> entry : cluster.entrySet()) {
            ClusterStatus status = new ClusterStatus();
            Jedis jedis = entry.getValue();
            //get cluster nodes...
            //<id> <ip:port> <flags> <master> <ping-sent> <pong-recv> <config-epoch> <link-state> <slot> <slot> ... <slot>
            //we only check myself node to get detailed info...
            try {
                String str_nodes = "";

                try {
                    str_nodes = jedis.clusterNodes();
                }
                catch(JedisDataException ex) {
                    status.setIp_port(entry.getKey());
                    status.setOK(false);
                    cluster_status_map.put(entry.getKey(), status);
                    logger.error(String.format("Exception|%s|%s",entry.getKey(), ex.getMessage()));
                    continue;
                }
                for (String node_info : str_nodes.split("\n")) {
                    if (node_info.contains("myself")) {
                        String[] node_status = node_info.split(" ");
                        if (node_status.length < 8)  //slave = 8, master = 9+
                        {
                            throw new JedisHelperException(String.format("[ERROR] Node|%s", node_info));
                        } else {
                            status.setNodeid(node_status[0]);
                            status.setIp_port(node_status[1]);
                            if (node_status[2].contains("master")) {
                                status.setMaster(true);
                                status.setOK(true);
                                //check slot
                                TreeSet<Integer> running_slots = new TreeSet<>();
                                TreeSet<Integer> importing_slots = new TreeSet<>();
                                TreeSet<Integer> migrating_slots = new TreeSet<>();
                                for (int i = 8; i < node_status.length; i++) {
                                    if (node_status[i].startsWith("[")) {  //importing/migrating slot
                                        int idx =  node_status[i].indexOf("-<-");
                                        if(idx > 0) {
                                            importing_slots.add(Integer.parseInt(node_status[i].substring(1, idx)));
                                        }
                                        idx = node_status[i].indexOf("->-");
                                        if(idx > 0) {
                                            migrating_slots.add(Integer.parseInt(node_status[i].substring(1, idx)));
                                        }
                                        setMigrating(true);
                                    } else {
                                        int idx = node_status[i].indexOf("-");
                                        if (idx > 0) {
                                            int start = Integer.parseInt(node_status[i].substring(0, idx));
                                            int end = Integer.parseInt(node_status[i].substring(idx + 1));
                                            for (int j = start; j <= end; j++) {
                                                running_slots.add(j);
                                            }
                                        } else {
                                            running_slots.add(new Integer(node_status[i]));
                                        }
                                    }
                                }
                                status.setRunning_slots(running_slots);
                                status.setImporting_slots(importing_slots);
                                status.setMigrating_slots(migrating_slots);
                                master_nodes.put(entry.getKey(), running_slots);
                                allocated_slots.addAll(running_slots);
                            } else if (node_status[2].contains("slave")) {
                                status.setMaster(false);
                                status.setOK(true);
                                status.setMaster_nodeid(node_status[3]);
                            } else {
                                status.setOK(false);
                                logger.error(str_nodes);
                            }
                            cluster_status_map.put(entry.getKey(), status);
                            nodeid_status_map.put(status.getNodeid(), status);
                        }
                        break;
                    }
                }
            } catch (JedisConnectionException e) {
                logger.error("Exception|", e);
                status.setOK(false);
                cluster_status_map.put(entry.getKey(), status);
            }
        }

        if(allocated_slots.size() == JedisCluster.HASHSLOTS)
            setCreated(true);

        for(Map.Entry<String, ClusterStatus> entry : cluster_status_map.entrySet()) {
            ClusterStatus status = entry.getValue();
            String s;
            if(status.isOK()) {
                if (status.isMaster())
                    s = "M ";
                else
                    s = "S ";
                s += status.getIp_port();
                if (!status.isMaster()) {
                    ClusterStatus master_status = nodeid_status_map.get(status.getMaster_nodeid());
                    if(master_status != null)
                    {
                        s += " " + master_status.getIp_port();
                        status.setMaster_ip(master_status.getIp_port());
                    }
                    else
                    {
                        s += " master error";
                    }
                }
                else {   //slave don't have slots...
                    s += " " + Integer.toString(status.getRunning_slots().size());
                }
            }
            else
            {
                s = "FAIL " + entry.getKey();
            }
            logger.info(s);
        }
        return cluster_status_map;
    }


    public static boolean waitForClusterReady(ArrayList<Jedis> values) throws InterruptedException {
        boolean clusterOk = false;
        int times = 0;
        while (!clusterOk && times <= 400 ) {    //50 * 400 = 20s
            boolean isOk = true;
            for (Jedis node : values) {
                if (!node.clusterInfo().split("\n")[0].contains("ok")) {
                    isOk = false;
                    break;
                }
            }
            if (isOk) {
                clusterOk = true;
            }
            Thread.sleep(50);
            times++;
        }
        return clusterOk;
    }

    public static String getNodeId(Jedis jedis) {
        for (String infoLine : jedis.clusterNodes().split("\n")) {
            if (infoLine.contains("myself")) {
                return infoLine.split(" ")[0];
            }
        }
        return "";
    }

    public void AddSet(ArrayList<String> ips) {
        Logger logger = Logger.getLogger(JedisHelper.class);
        //Add master first, then add slave
        //1.Check status, and get copy, status OK
        CheckStatus();
        if(isMigrating()) {
            updateStatus("[ERROR] Another Migration is in progress.");
            return;
        }
        //2.ClusterMeet
        if(ips.size() % copy != 0)
        {
            updateStatus("[ERROR] Input IP number.");
            return;
        }
        else {
            try {
                for(int c = 0; c < ips.size()/copy; c++) {
                    Map.Entry<String, Jedis> entry = cluster.entrySet().iterator().next();
                    String node_id = "";
                    for (int i = 0; i < copy; i++) {
                        String[] ip_pair = ips.get(c * copy + i).split(":");
                        Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                        jedis.connect();
                        if (i == 0)
                            node_id = getNodeId(jedis);
                        cluster.put(ips.get(c * copy + i), jedis);
                        entry.getValue().clusterMeet(ip_pair[0], Integer.parseInt(ip_pair[1]));
                        updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Cluster meets.");
                    }
                    //3. waitForclusterReady
                    if(!waitForClusterReady(new ArrayList<Jedis>(cluster.values()))) {
                        updateStatus("[ERROR] Cluster meet fails.");
                        return;
                    }
                    //4. Set replicates
                    if (!node_id.isEmpty()) {
                        for (int i = 1; i < copy; i++) {
                            cluster.get(ips.get(c * copy + i)).clusterReplicate(node_id);
                            String[] ip_pair = ips.get(c * copy + i).split(":");
                            updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Cluster replicates.");
                        }
                    }
                }
                //5. create sharding plan
                ArrayList<ShardingPlanInfo> sharding_plan = ShardingPlanAdd();
                //6. migrating data
                Migrate(sharding_plan, 1);
                updateStatus("Done.");
            } catch (JedisConnectionException e) {
                logger.error("Exception|", e);
                updateStatus("[ERROR] Connection.");
            } catch (InterruptedException e) {
                logger.error("Exception|", e);
                updateStatus("[ERROR] Interrupted.");
            }
        }
    }

    //Input: IP:Port(M);IP:Port(S);...
    public void CreateSet(ArrayList<String> ips) {
        Logger logger = Logger.getLogger(JedisHelper.class);
        //Add master first, then add slave
        //1.Check status, and get copy, status OK
        CheckStatus();
        if (isMigrating() || isCreated()) {
            updateStatus("[ERROR] Already created.");
            return;
        }

        //2.ClusterMeet
        if(ips.size() % copy != 0)
        {
            updateStatus("[ERROR] Input IP number.");
            return;
        }
        else {
            boolean FirstConnect = false;
            if(cluster.size()%copy != 0) {
                if(cluster.size() == 1 && ips.get(0).equals(cluster.keySet().iterator().next())) { //first IP should match
                    FirstConnect = true;
                    String[] ip_pair = ips.get(0).split(":");
                    updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Cluster meets.");
                }
                else {
                    updateStatus("[ERROR] Input IP.");
                }
            }
            try {
                //2. ClusterMeet master
                ArrayList<Jedis> masters = new ArrayList<>();
                for(String ip: master_nodes.keySet()) {
                    masters.add(cluster.get(ip));
                }
                for(int c = 0; c < ips.size()/copy; c++) {
                    Map.Entry<String, Jedis> entry = cluster.entrySet().iterator().next();
                    if(!FirstConnect || c != 0) {
                        String[] ip_pair = ips.get(c * copy).split(":");
                        Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                        jedis.connect();
                        masters.add(jedis);
                        entry.getValue().clusterMeet(ip_pair[0], Integer.parseInt(ip_pair[1]));
                        updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Cluster meets.");
                        cluster.put(ips.get(c * copy), jedis);
                    }
                }
                int begin_slot_index = 0;
                int num = JedisCluster.HASHSLOTS / masters.size();
                int div = JedisCluster.HASHSLOTS % masters.size();

                logger.info(String.format("Create|%d|%d|%d|%d", ips.size(), cluster.size(), copy,masters.size()));

                for( int i = 0; i < masters.size(); i++) {
                    //div == 0: slot_num = num
                    //div != 0: if x < master_nodes.size()-div, slot_x_num = num; else slot_x_num = num+1;
                    int slot_num = num;
                    if (div != 0 && i >= (masters.size() - div)) {
                        slot_num = num + 1;
                    }
                    int[] slots = new int[slot_num];
                    for(int j = 0; j < slot_num; j++) {
                        slots[j] = begin_slot_index + j;
                    }
                    masters.get(i).clusterAddSlots(slots);
                    begin_slot_index += slot_num;
                }

                if(!waitForClusterReady(masters)) {
                    updateStatus("[ERROR] Cluster meet fails.");
                    return;
                }

                //3. ClusterMeet slaves
                for(int c = 0; c < ips.size()/copy; c++) {
                    Map.Entry<String, Jedis> entry = cluster.entrySet().iterator().next();
                    String node_id = "";
                    for (int i = 0; i < copy; i++) {
                        logger.info(String.format("cluster meet|%s", ips.get(c*copy+i)));
                        if (i == 0) {
                            node_id = getNodeId(cluster.get(ips.get(c*copy)));
                        }
                        else {
                            String[] ip_pair = ips.get(c * copy + i).split(":");
                            Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                            jedis.connect();
                            cluster.put(ips.get(c * copy + i), jedis);
                            entry.getValue().clusterMeet(ip_pair[0], Integer.parseInt(ip_pair[1]));
                            updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Cluster meets.");
                        }
                    }
                    logger.info("waitReady");
                    if(!waitForClusterReady(new ArrayList<Jedis>(cluster.values()))) {
                        updateStatus("[ERROR] Cluster meet fails.");
                        return;
                    }

                    //4. Set replicates
                    if (!node_id.isEmpty()) {
                        for (int i = 1; i < copy; i++) {
                            cluster.get(ips.get(c * copy + i)).clusterReplicate(node_id);
                            String[] ip_pair = ips.get(c * copy + i).split(":");
                            updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Set replicate.");
                        }
                    }
                }
                //6. Check status
                CheckStatus();
                if(isCreated()) {
                    updateStatus("Done.");
                }

            } catch (JedisConnectionException e) {
                logger.error("Exception|", e);
                for(int i = 0; i < ips.size(); i++) {
                    String[] ip_pair = ips.get(i).split(":");
                    updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "[ERROR] Connection.");
                }
            } catch (InterruptedException e) {
                logger.error("Exception|", e);
                for(int i = 0; i < ips.size(); i++) {
                    String[] ip_pair = ips.get(i).split(":");
                    updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "[ERROR] Interrupted.");
                }
            }
        }
    }

    public void close() {
        for(Jedis jedis : cluster.values())
            jedis.disconnect();
    }

    public ArrayList<ShardingPlanInfo> ShardingPlanAdd() {
        Logger logger = Logger.getLogger(JedisHelper.class);
        ArrayList<ShardingPlanInfo> sharding_plan = new ArrayList<>();
        HashMap<String, ClusterStatus> cluster_map = CheckStatus();
        int num = JedisCluster.HASHSLOTS / master_nodes.size();
        int div = JedisCluster.HASHSLOTS % master_nodes.size();
        //from least to most slot num
        List<Map.Entry<String,TreeSet<Integer>>> node_list = new ArrayList<Map.Entry<String,TreeSet<Integer>>>(master_nodes.entrySet());
        Collections.sort(node_list, new Comparator<Map.Entry<String, TreeSet<Integer>>>() {
            //…˝–Ú≈≈–Ú
            public int compare(Map.Entry<String, TreeSet<Integer>> o1,
                               Map.Entry<String, TreeSet<Integer>> o2) {
                return new Integer(o1.getValue().size()).compareTo(o2.getValue().size());
            }
        });
        /*
        logger.info("before");
        for(Map.Entry<String, TreeSet<Integer> > node : node_list) {
            logger.info(node.getKey());
            logger.info(node.getValue().size());
        }*/
        for(int i = 0, j = node_list.size()-1; i < j; i++) {
            //div == 0: slot_num = num
            //div != 0: if x <= div-1,.slot_x_num = num; else slot_x_num = num+1;
            int slot_i_num = num;
            int slot_j_num = num;
            if(div != 0) {
                if(i >= (node_list.size()-div))
                    slot_i_num = num+1;
                if(j >= (node_list.size()-div))
                    slot_j_num = num+1;
            }

            ShardingPlanInfo plan = new ShardingPlanInfo();
            while(node_list.get(i).getValue().size() < slot_i_num) {
                if (node_list.get(j).getValue().size() > slot_j_num) {
                    //move to i
                    int first = node_list.get(j).getValue().first();
                    plan.getSlot_id().add(first);
                    node_list.get(i).getValue().add(first);
                    node_list.get(j).getValue().remove(first);
                } else {
                    if(!plan.getSlot_id().isEmpty()) {
                        logger.info(String.format("%d|%d|%d|%d", i, j, node_list.get(i).getValue().size(), node_list.get(j).getValue().size()));
                        plan.setFrom_ip(node_list.get(j).getKey());
                        plan.setTo_ip(node_list.get(i).getKey());
                        sharding_plan.add(plan);
                    }

                    j--;
                    if(j == i)
                        break;  //Œﬁ–ÎºÃ–¯∑÷≈‰¡À
                    plan = new ShardingPlanInfo();
                    if(j >= (node_list.size()-div) && div != 0)
                        slot_j_num = num+1;
                    else
                        slot_j_num = num;
                }
            }
            if(!plan.getSlot_id().isEmpty()) {
                logger.info(String.format("%d|%d|%d|%d", i, j, node_list.get(i).getValue().size(), node_list.get(j).getValue().size()));
                plan.setFrom_ip(node_list.get(j).getKey());
                plan.setTo_ip(node_list.get(i).getKey());
                sharding_plan.add(plan);
            }
        }
        /*
        logger.info("after");
        for(Map.Entry<String, TreeSet<Integer> > node : node_list) {
            logger.info(node.getKey());
            logger.info(node.getValue().size());
        }
        for(ShardingPlanInfo p : sharding_plan) {
            logger.info(String.format("%s|%s|%d", p.getFrom_ip(), p.getTo_ip(), p.getSlot_id().size()));
        }
        */
        return sharding_plan;
    }

    public ArrayList<ShardingPlanInfo> ShardingPlanRemove(ArrayList<String> ips) {
        Logger logger = Logger.getLogger(JedisHelper.class);
        ArrayList<ShardingPlanInfo> sharding_plan = new ArrayList<>();
        HashMap<String, TreeSet<Integer> > remove_nodes = new HashMap<>();
        HashMap<String, ClusterStatus> cluster_map = CheckStatus();
        for(String ip: ips)
            remove_nodes.put(ip, master_nodes.remove(ip));
        int num = JedisCluster.HASHSLOTS / (master_nodes.size());
        int div = JedisCluster.HASHSLOTS % (master_nodes.size());
        //from least to most slot num
        List<Map.Entry<String,TreeSet<Integer>>> node_list = new ArrayList<Map.Entry<String,TreeSet<Integer>>>(master_nodes.entrySet());
        Collections.sort(node_list, new Comparator<Map.Entry<String, TreeSet<Integer>>>() {
            //…˝–Ú≈≈–Ú
            public int compare(Map.Entry<String, TreeSet<Integer>> o1,
                               Map.Entry<String, TreeSet<Integer>> o2) {
                return new Integer(o1.getValue().size()).compareTo(o2.getValue().size());
            }
        });
        /*
        logger.info("before");
        for(Map.Entry<String, TreeSet<Integer> > node : node_list) {
            logger.info(node.getKey());
            logger.info(node.getValue().size());
        }
        */
        for(int i = 0; i < node_list.size(); i++) {
            //div == 0: slot_num = num
            //div != 0: if x < node_list.size()-div, slot_x_num = num; else slot_x_num = num+1;
            int slot_num = num;
            if(div != 0 && i >= (node_list.size()-div)) {
                slot_num = num+1;
            }

            ShardingPlanInfo plan = new ShardingPlanInfo();
            Iterator<Map.Entry<String, TreeSet<Integer>>> it = remove_nodes.entrySet().iterator();
            Map.Entry<String, TreeSet<Integer>> e = it.next();
            while(node_list.get(i).getValue().size() < slot_num) {
                if (e.getValue().size() > 0) {
                    //move to i
                    int first = e.getValue().first();
                    plan.getSlot_id().add(first);
                    node_list.get(i).getValue().add(first);
                    e.getValue().remove(first);
                } else {
                    if(!plan.getSlot_id().isEmpty()) {
                        logger.info(String.format("%d|%d|%d", i, node_list.get(i).getValue().size(), e.getValue().size()));
                        plan.setFrom_ip(e.getKey());
                        plan.setTo_ip(node_list.get(i).getKey());
                        sharding_plan.add(plan);
                    }
                    if(it.hasNext()) {
                        plan = new ShardingPlanInfo();
                        e = (Map.Entry<String, TreeSet<Integer>>) it.next();
                    }
                    else
                        break;
                }
            }
            if(!plan.getSlot_id().isEmpty()) {
                logger.info(String.format("%d|%d|%d", i, node_list.get(i).getValue().size(), e.getValue().size()));
                plan.setFrom_ip(e.getKey());
                plan.setTo_ip(node_list.get(i).getKey());
                sharding_plan.add(plan);
            }
        }
        /*
        logger.info("after");
        for(Map.Entry<String, TreeSet<Integer> > node : node_list) {
            logger.info(node.getKey());
            logger.info(node.getValue().size());
        }
        for(ShardingPlanInfo p : sharding_plan) {
            logger.info(String.format("%s|%s|%d", p.getFrom_ip(), p.getTo_ip(), p.getSlot_id().size()));
        }*/
        return sharding_plan;
    }

    public void Migrate(ArrayList<ShardingPlanInfo> sharding_plan, int op) {
        Logger logger = Logger.getLogger(JedisHelper.class);
        for(ShardingPlanInfo plan: sharding_plan) {
            Jedis from_node = cluster.get(plan.getFrom_ip());
            Jedis to_node = cluster.get(plan.getTo_ip());
            final String[] to_ip_pair = plan.getTo_ip().split(":");

            for(int i = 0; i < plan.getSlot_id().size(); i++) {
                int slot = plan.getSlot_id().get(i);
                to_node.clusterSetSlotImporting(slot, getNodeId(from_node));
                from_node.clusterSetSlotMigrating(slot, getNodeId(to_node));
                List<String> keys = from_node.clusterGetKeysInSlot(slot, 100);
                while(!keys.isEmpty()) {
                    /*
                    for( String key : keys) {
                        from_node.migrate(ip_pair[0], Integer.parseInt(ip_pair[1]), key, 0, 10000);  //TODO, using KEYS for batch migration.
                    }*/
                    from_node.migrate_keys(to_ip_pair[0], Integer.parseInt(to_ip_pair[1]), 0, 10000, keys.toArray(new String[keys.size()]));
                    keys = from_node.clusterGetKeysInSlot(slot, 100);
                }
                //Setslot node for master
                for(Map.Entry<String, Jedis> master_entry : cluster.entrySet()) {
                    if(master_nodes.containsKey(master_entry.getKey())) {
                        master_entry.getValue().clusterSetSlotNode(slot, getNodeId(to_node));
                    }
                }
                if(op == 1) {

                    if((i+1) == plan.getSlot_id().size()) {
                        updateStatus(to_ip_pair[0], Integer.parseInt(to_ip_pair[1]), String.format("Migrating from %s done.", plan.getFrom_ip()));
                    }
                    else
                        updateStatus(to_ip_pair[0], Integer.parseInt(to_ip_pair[1]), String.format("Migrating from %s(%d/%d).", plan.getFrom_ip(), i+1, plan.getSlot_id().size()));

                }
                else {
                    String[] from_ip_pair = plan.getFrom_ip().split(":");
                    if((i+1) == plan.getSlot_id().size()) {
                        updateStatus(from_ip_pair[0], Integer.parseInt(from_ip_pair[1]), String.format("Importing to %s done.", plan.getTo_ip()));
                    }
                    else
                        updateStatus(from_ip_pair[0], Integer.parseInt(from_ip_pair[1]), String.format("Importing to %s(%d/%d).", plan.getTo_ip(), i + 1, plan.getSlot_id().size()));
                }

                //logger.info(String.format("%s: Slot %d migrated.", plan.getTo_ip(), slot));
            }
        }
    }

    //Input ArrayList String: IP:Port(M)
    public void RemoveSet(ArrayList<String> ips) throws JedisHelperException {
        Logger logger = Logger.getLogger(JedisHelper.class);
        //1.Check status, replicate, hash slot empty and remove slave first
        HashMap<String, ClusterStatus> cluster_map = CheckStatus();

        try {
            for(String ip: ips) {
                if(!master_nodes.containsKey(ip)) {
                    updateStatus(String.format("[ERROR] Server not master:%s", ip));
                    return;
                }
            }
            //1. migrate slot..
            ArrayList<ShardingPlanInfo> sharding_plan = ShardingPlanRemove(ips);
            Migrate(sharding_plan, 2);

            for(String ip: ips) {
                ClusterStatus status = cluster_map.get(ip);
                if(status != null && status.isMaster()) {
                    //2. get slave and remove
                    for(ClusterStatus slave_status : cluster_map.values())
                    {
                        if(!slave_status.isMaster() && slave_status.getMaster_nodeid().equals(status.getNodeid())) {
                            for (Map.Entry<String, Jedis> entry : cluster.entrySet()) {
                                if(!entry.getKey().equals(slave_status.getIp_port())) {
                                    entry.getValue().clusterForget(slave_status.getNodeid());
                                }
                            }
                            cluster.get(slave_status.getIp_port()).shutdown();
                            cluster.remove(slave_status.getIp_port());
                            String[] ip_pair = slave_status.getIp_port().split(":");
                            updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Done.");
                        }
                    }
                    //3. finally remove master
                    for (Map.Entry<String, Jedis> entry : cluster.entrySet()) {
                        if(!entry.getKey().equals(status.getIp_port())) {
                            entry.getValue().clusterForget(status.getNodeid());
                        }
                        else {
                            entry.getValue().shutdown();    //shutdown the server
                        }
                    }
                    cluster.get(status.getIp_port()).shutdown();
                    cluster.remove(status.getIp_port());
                    String[] ip_pair = status.getIp_port().split(":");
                    updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Done.");
                }
                else {
                    logger.info(String.format("ip not master:%s", ip));
                }
            }
            CheckStatus();
        }
        catch (JedisConnectionException e) {
            logger.error("Exception|", e);
            throw e;
        }
    }

    //Input ArrayList String: IP:Port(M)
    public void RecoverSet(ArrayList<String> ips) throws JedisHelperException {
        Logger logger = Logger.getLogger(JedisHelper.class);
        //1.Check status, replicate, hash slot empty and remove slave first
        HashMap<String, ClusterStatus> cluster_map = CheckStatus();

        try {
            //1. remove fail servers
            for(String node: fail_nodes) {
                for (Map.Entry<String, Jedis> entry : cluster.entrySet()) {
                    if(entry.getValue().clusterNodes().contains(node)) {
                        entry.getValue().clusterForget(node);
                        logger.info(String.format("%s|forget|%s", entry.getKey(), node));
                    }
                }
            }
            //2. add new ip
            for(String host: ips) {
                String[] ip_pair = host.split(":");
                Jedis jedis = new Jedis(ip_pair[0], Integer.parseInt(ip_pair[1]));
                jedis.connect();
                cluster.put(host, jedis);
                logger.info("clusterMeet|" + ip + ":" + port + "|" + host + "|" + cluster.get(ip + ":" + port));
                cluster.get(ip + ":" + port).clusterMeet(ip_pair[0], Integer.parseInt(ip_pair[1]));
                updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Cluster meets.");
            }
            //3. waitForClusterReady
            logger.info("waitReady|"+cluster.values().size());
            if(!waitForClusterReady(new ArrayList<Jedis>(cluster.values()))) {
                updateStatus("[ERROR] Cluster meet fails.");
                return;
            }
            //4. clusterReplicate
            String node_id = getNodeId(cluster.get(ip + ":" + port));
            for(String host: ips) {
                cluster.get(host).clusterReplicate(node_id);
                String[] ip_pair = host.split(":");
                updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "Done.");
            }
            CheckStatus();
        }
        catch (JedisConnectionException e) {
            logger.error("Exception|", e);
            throw e;
        }
        catch (InterruptedException e) {
            logger.error("Exception|", e);
            for(int i = 0; i < ips.size(); i++) {
                String[] ip_pair = ips.get(i).split(":");
                updateStatus(ip_pair[0], Integer.parseInt(ip_pair[1]), "[ERROR] Interrupted.");
            }
        }
    }

    public String getIp() {
        return ip;
    }

    public void setIp(String ip) {
        this.ip = ip;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }

    public HashMap<String, Jedis> getCluster() {
        return cluster;
    }

    public void setCluster(HashMap<String, Jedis> cluster) {
        this.cluster = cluster;
    }

    public int getCopy() {
        return copy;
    }

    public void setCopy(int copy) {
        this.copy = copy;
    }

    public HashMap<String, TreeSet<Integer>> getMaster_nodes() {
        return master_nodes;
    }

    public void setMaster_nodes(HashMap<String, TreeSet<Integer>> master_nodes) {
        this.master_nodes = master_nodes;
    }

    public boolean isMigrating() {
        return migrating;
    }

    public void setMigrating(boolean migrating) {
        this.migrating = migrating;
    }

    public boolean isCreated() {
        return created;
    }

    public void setCreated(boolean created) {
        this.created = created;
    }

    public boolean isOK() {
        return OK;
    }

    public void setOK(boolean OK) {
        this.OK = OK;
    }

    public boolean isChanged() {
        return changed;
    }

    public void setChanged(boolean changed) {
        this.changed = changed;
    }

    public String getError_message() {
        return error_message;
    }

    public void setError_message(String error_message) {
        this.error_message = error_message;
    }

    String plan_id;
    String ip;
    int port;
    HashMap<String, ServerInfo> host_map;
    String first_level_service_name;
    String second_level_service_name;
    ClusterInfo cluster_info;
    HashMap<String, Jedis> cluster;
    ArrayList<String> fail_nodes;
    int copy;
    HashMap<String, TreeSet<Integer> > master_nodes;
    TreeSet<Integer> allocated_slots;

    //for detail...
    HashMap<String, ClusterStatus> cluster_status_map;
    HashMap<String, ArrayList<String>> master_acks;
    HashMap<String, String> slot_sigs;
    HashMap<String, ArrayList<String>> master_slave_infos;

    boolean migrating;
    boolean created;
    boolean OK;
    boolean changed;
    String error_message;

    public final int CLUSTER_HASH_SLOTS = 16384;
}
