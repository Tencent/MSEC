
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
import beans.request.QueryESClusterDetailRequest;
import beans.response.QueryESClusterDetailResponse;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import org.apache.log4j.Logger;

import java.lang.reflect.Array;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class QueryESClusterDetail extends JsonRPCHandler {
    DBUtil util;
    ArrayList<String> cluster_ips;

    private void getStatus(String first_level_service_name, String second_level_service_name) throws  SQLException {
        Logger logger = Logger.getLogger(QueryESClusterDetail.class);
        String sql = "select ip, port from t_service_info where first_level_service_name=? and second_level_service_name=?";
        List<Object> params = new ArrayList<Object>();
        params.add(first_level_service_name);
        params.add(second_level_service_name);

        ArrayList<Map<String, Object>> result_list = util.findModeResult(sql, params);

        for(int i = 0; i < result_list.size(); i++){
            cluster_ips.add(result_list.get(i).get("ip").toString());
        }
        logger.info(cluster_ips);
    }

    private void updateStatus(String first_level_service_name, String second_level_service_name, String ip, boolean isOK) throws SQLException
    {
        Logger logger = Logger.getLogger(QueryESClusterDetail.class);

        String sql = "";
        List<Object> params = new ArrayList<Object>();
        if(isOK) {
            sql = "update t_service_info set status=? where first_level_service_name=? and second_level_service_name=? and ip=?";
            params.add("OK");
        }
        else
        {
            sql = "update t_service_info set status=? where first_level_service_name=? and second_level_service_name=? and ip=?";
            params.add("ERROR");
        }

        params.add(first_level_service_name);
        params.add(second_level_service_name);
        params.add(ip);

        int updNum = util.updateByPreparedStatement(sql, params);
        if (updNum != 1) {
            logger.error(String.format("updateStatus|%d", updNum));
        }
    }

    private void addServer(String first_level_service_name, String second_level_service_name, String ip, int port) throws SQLException
    {
        Logger logger = Logger.getLogger(QueryESClusterDetail.class);

        String sql = "";
        List<Object> params = new ArrayList<Object>();
        sql = "insert into t_service_info(first_level_service_name, second_level_service_name, ip, port) values(?,?,?,?)";
        params.clear();
        params.add(first_level_service_name);
        params.add(second_level_service_name);
        params.add(ip);
        params.add(port);
        try {
            util.updateByPreparedStatement(sql, params);
        } catch (SQLException e) {
            logger.error(e);
        }
    }

    private boolean updatePlan(String first_level_service_name, String second_level_service_name) throws SQLException {
        Logger logger = Logger.getLogger(QueryESClusterDetailResponse.class);
        String sql = "select distinct status from t_install_plan where plan_id=(select plan_id from t_second_level_service where first_level_service_name=? and second_level_service_name=?)";
        List<Object> params = new ArrayList<Object>();
        params.add(first_level_service_name);
        params.add(second_level_service_name);
        ArrayList<Map<String, Object>> status_infos = util.findModeResult(sql, params);
        if(status_infos.size() == 0)
            return false;
        boolean need_refresh = true;
        for(int i = 0; i < status_infos.size(); i++) {
            if (!status_infos.get(i).get("status").toString().startsWith("[ERROR]") && !status_infos.get(i).get("status").toString().startsWith("Done")) {
                need_refresh = false;
                break;
            }
        }
        return need_refresh;
    }

    private void updateMaster(String first_level_service_name, String second_level_service_name, String ip, int port, int group_id) throws SQLException
    {
        Logger logger = Logger.getLogger(QueryESClusterDetailResponse.class);

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

    public QueryESClusterDetailResponse exec(QueryESClusterDetailRequest request)
    {
        Logger logger = Logger.getLogger(QueryESClusterDetail.class);
        QueryESClusterDetailResponse resp = new QueryESClusterDetailResponse();

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

        cluster_ips  = new ArrayList<>();
        HashMap<String, QueryESClusterDetailResponse.RTInfo> info_map = new HashMap<>();

        try {
            if(updatePlan(request.getFirst_level_service_name(), request.getSecond_level_service_name())) {
                resp.setMessage("refresh");
                resp.setStatus(101);
                return resp;
            }

            getStatus(request.getFirst_level_service_name(), request.getSecond_level_service_name());

            ESHelper helper = new ESHelper();
            helper.ClusterStatus(cluster_ips, request.getFirst_level_service_name()+"."+request.getSecond_level_service_name(), resp);
            if(resp.getHealth_status() != "INIT") {
                for(Map.Entry<String, QueryESClusterDetailResponse.RTInfo> entry: resp.getInfo_map().entrySet()) {
                    if(!cluster_ips.contains(entry.getKey())) {
                        //add ip to service
                        addServer(request.getSecond_level_service_name(), request.getSecond_level_service_name(), entry.getKey(), resp.getServer_port());
                        resp.setMessage("refresh");
                        resp.setStatus(101);
                        return resp;
                    }
                }

                for (String ip : cluster_ips) {
                    QueryESClusterDetailResponse.RTInfo rt_info = resp.getInfo_map().get(ip);
                    if (rt_info != null) {
                        if (!rt_info.isOK()) {
                            logger.error(String.format("ip failed|%s", ip));
                        }
                        updateStatus(request.getFirst_level_service_name(), request.getSecond_level_service_name(), ip, rt_info.isOK());
                    } else {
                        resp.getInfo_map().put(ip, new QueryESClusterDetailResponse().new RTInfo());
                        updateStatus(request.getFirst_level_service_name(), request.getSecond_level_service_name(), ip, false);
                    }
                }


            }
        }
        catch (Exception e) {
            logger.error(e);
            e.printStackTrace();
        } finally {
            util.releaseConn();
        }

        return resp;
    }
}
