
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
import beans.response.QueryESClusterDetailResponse;
import msec.org.DBUtil;
import org.apache.log4j.Logger;
import org.elasticsearch.ElasticsearchTimeoutException;
import org.elasticsearch.action.admin.cluster.health.ClusterHealthResponse;
import org.elasticsearch.action.admin.cluster.node.info.NodeInfo;
import org.elasticsearch.action.admin.cluster.node.info.NodesInfoResponse;
import org.elasticsearch.action.admin.cluster.node.stats.NodeStats;
import org.elasticsearch.action.admin.cluster.node.stats.NodesStatsResponse;
import org.elasticsearch.action.admin.cluster.stats.ClusterStatsNodes;
import org.elasticsearch.action.admin.indices.template.put.PutIndexTemplateRequestBuilder;
import org.elasticsearch.action.admin.indices.template.put.PutIndexTemplateResponse;
import org.elasticsearch.client.transport.NoNodeAvailableException;
import org.elasticsearch.client.transport.TransportClient;
import org.elasticsearch.cluster.health.ClusterHealthStatus;
import org.elasticsearch.common.settings.Settings;
import org.elasticsearch.common.transport.InetSocketTransportAddress;
import org.elasticsearch.common.unit.TimeValue;
import org.elasticsearch.discovery.MasterNotDiscoveredException;
import org.elasticsearch.transport.client.PreBuiltTransportClient;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.*;
import java.util.concurrent.TimeUnit;

public class ESHelper {
    private static final String MAPPING = "{\n" +
        "        \"logs\" : {\n" +
        "            \"_all\" : { \"enabled\" : false },\n" +
        "            \"properties\" : {\n" +
        "               \"ClientIP\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"not_analyzed\"\n" +
        "                },\n" +
        "               \"IP\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"not_analyzed\"\n" +
        "                },\n" +
        "               \"ServerIP\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"not_analyzed\"\n" +
        "                },\n" +
        "               \"FileLine\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"not_analyzed\"\n" +
        "                },\n" +
        "               \"Level\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"not_analyzed\"\n" +
        "                },\n" +
        "               \"Caller\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"not_analyzed\"\n" +
        "                },\n" +
        "               \"ServiceName\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"not_analyzed\"\n" +
        "                },\n" +
        "               \"RPCName\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"not_analyzed\"\n" +
        "                },\n" +
        "               \"ReqID\" : {\n" +
        "                   \"type\" : \"long\"\n" +
        "                },\n" +
        "               \"InsTime\" : {\n" +
        "                   \"type\" : \"date\"\n" +
        "                },\n" +
        "               \"body\" : {\n" +
        "                   \"type\" : \"string\",\n" +
        "                   \"index\" : \"no\"\n" +
        "                }\n" +
        "            }\n" +
        "        }\n" +
        "    }";

    public class ESHelperException extends RuntimeException {
        public ESHelperException(String message) {
            super(message);
        }

        public ESHelperException(Throwable e) {
            super(e);
        }

        public ESHelperException(String message, Throwable cause) {
            super(message, cause);
        }
    }

    public ESHelper() {}

    public ESHelper(ClusterInfo cluster_info_)
    {
        cluster_info = cluster_info_;
    }

    private void updateStatus(String ip, String status)
    {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "update t_install_plan set status=? where plan_id=? and ip=?";
            List<Object> params = new ArrayList<Object>();
            params.add(status);
            params.add(cluster_info.getPlan_id());
            params.add(ip);

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum != 1) {
                return;
            }
        } catch (Exception e) {
            logger.error(e);
            logger.error(e);
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
            params.add(cluster_info.getPlan_id());

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum < 0) {
                return;
            }
        } catch (Exception e) {
            logger.error(e);
            logger.error(e);
            return;
        } finally {
            util.releaseConn();
        }
    }

    private void updateStatus_waitforcluster(String status)
    {
        Logger logger = Logger.getLogger(InstallServerProc.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null) {
            return;
        }

        try {
            String sql = "update t_install_plan set status=? where plan_id=? and status=?";
            List<Object> params = new ArrayList<Object>();
            params.add(status);
            params.add(cluster_info.getPlan_id());
            params.add(waitforcluster_message);

            int updNum = util.updateByPreparedStatement(sql, params);
            if (updNum < 0) {
                return;
            }
        } catch (Exception e) {
            logger.error(e);
            logger.error(e);
            return;
        } finally {
            util.releaseConn();
        }
    }

    public void waitForClusterReady(final TransportClient client, ArrayList<String> ips, final ClusterHealthStatus status) throws IOException {
        Logger logger = Logger.getLogger(ESHelper.class);
        int timeout = 60;
        int node_num = 0;
        long begin = System.currentTimeMillis() / 1000L;
        long end = begin;
        Set<String> done_ips = new HashSet<>();
        try {
            logger.info("waiting for cluster state: " + status.name());
            ClusterHealthResponse healthResponse = null;
            while(true) {
                try {
                    healthResponse = client.admin().cluster().prepareHealth().setWaitForStatus(status)
                            .setTimeout(TimeValue.timeValueSeconds(5)).execute().actionGet();
                }
                catch(NoNodeAvailableException | MasterNotDiscoveredException ex)
                {
                    end = System.currentTimeMillis() / 1000L;
                    if(end-begin >= timeout)
                        throw new IOException("Server start timeout");
                    logger.info("server still starting/discovering, retry...");
                    try {
                        TimeUnit.SECONDS.sleep(2);
                    }
                    catch(InterruptedException e)
                    {
                    }
                    continue;
                }
                if (healthResponse != null && healthResponse.isTimedOut()) {
                    end = System.currentTimeMillis() / 1000L;
                    if(end-begin >= timeout) //timeout
                        throw new IOException("cluster not ready, current state is " + healthResponse.getStatus().name());
                    continue;
                } else {
                    logger.info("cluster state ok");
                    int new_node_num = healthResponse.getNumberOfNodes();
                    if (new_node_num > node_num) {
                        node_num = new_node_num;
                        NodesInfoResponse nodeInfos = client.admin().cluster().prepareNodesInfo().all().get();
                        for (NodeInfo node : nodeInfos.getNodes()) {
                            if (!done_ips.contains(node.getHostname()) && ips.contains(node.getHostname())) {
                                updateStatus(node.getHostname(), "Done.");
                                done_ips.add(node.getHostname());
                            }
                        }
                        if(done_ips.size() == ips.size())
                            break;

                        end = System.currentTimeMillis() / 1000L;
                        if(end-begin >= timeout) //timeout
                            break;
                    }
                }
            }
        } catch (final ElasticsearchTimeoutException e) {
            throw new IOException("ES API timeout");
        }
    }

    public void ClusterAdd(ArrayList<String> ips) {
        Logger logger = Logger.getLogger(ESHelper.class);
        TransportClient client = null;
        try {
            Settings settings = Settings.builder().put("cluster.name", cluster_info.getCluster_name()).put("client.transport.sniff", true).build();
            for(String ip : ips) {
                if(client == null) {
                    client = new PreBuiltTransportClient(settings).addTransportAddress(new InetSocketTransportAddress(InetAddress.getByName(ip), default_client_port));
                }
                else
                    client.addTransportAddress(new InetSocketTransportAddress(InetAddress.getByName(ip), default_client_port));
            }
            updateStatus(waitforcluster_message);
            waitForClusterReady(client, ips, ClusterHealthStatus.YELLOW);
        }
        catch(UnknownHostException e) {
            logger.error(e);
            updateStatus("[ERROR] Connect failed.");
        }
        catch(IOException e) {
            logger.error(e);
            updateStatus_waitforcluster("[ERROR] Joining cluster failed.");
        }
        finally {
            if(client!=null)
                client.close();
        }
    }

    public void ClusterStatus(ArrayList<String> ips, String cluster_name, QueryESClusterDetailResponse response ) {
        Logger logger = Logger.getLogger(ESHelper.class);
        TransportClient client = null;
        HashSet<String> total_ips = new HashSet<>(ips);
        try {
            Settings settings = Settings.builder().put("cluster.name", cluster_name).put("client.transport.sniff", true).build();
            for(String ip : ips) {
                if(client == null) {
                    client = new PreBuiltTransportClient(settings).addTransportAddress(new InetSocketTransportAddress(InetAddress.getByName(ip), default_client_port));
                }
                else
                    client.addTransportAddress(new InetSocketTransportAddress(InetAddress.getByName(ip), default_client_port));
            }
            ClusterHealthResponse healthResponse = client.admin().cluster().prepareHealth().setTimeout(TimeValue.timeValueSeconds(5)).execute().actionGet();
            if (healthResponse != null && !healthResponse.isTimedOut()) {
                response.setActive_shards(healthResponse.getActiveShards());
                response.setTotal_shards(healthResponse.getActiveShards()+healthResponse.getUnassignedShards());
                response.setHealth_status(healthResponse.getStatus().toString().toLowerCase());
                response.setServer_port(default_port);
                logger.info(healthResponse);
                if(healthResponse.getNumberOfNodes() > 0) {
                    NodesStatsResponse nodeStats = client.admin().cluster().prepareNodesStats().all().get();
                    if (nodeStats != null) {
                        for(NodeStats stats: nodeStats.getNodes()) {
                            QueryESClusterDetailResponse.RTInfo info  = new QueryESClusterDetailResponse().new RTInfo();
                            info.setOK(true);
                            info.setAvail_disk_size(stats.getFs().getTotal().getAvailable().getBytes());
                            info.setDoc_count(stats.getIndices().getDocs().getCount());
                            info.setDoc_disk_size(stats.getIndices().getStore().getSizeInBytes());
                            response.getInfo_map().put(stats.getNode().getAddress().getHost(), info);
                            total_ips.add(stats.getNode().getAddress().getHost());
                        }
                    }
                }
            }
            //update Zen settings
            client.admin().cluster().prepareUpdateSettings()
                        .setPersistentSettings(Settings.builder().put("discovery.zen.minimum_master_nodes", total_ips.size()/2+1))
                        .setTransientSettings(Settings.builder().put("discovery.zen.minimum_master_nodes", total_ips.size()/2+1))
                        .get();

            //update template settings
            PutIndexTemplateResponse tpl_resp =  client.admin().indices().preparePutTemplate("template_msec")
                    .setTemplate("msec_*")
                    .setSettings(
                            Settings.builder()
                                    .put("number_of_replicas", 1)
                                    .put("refresh_interval", "30s")
                    )
                    .addMapping("logs", MAPPING)
                    .execute()
                    .get();
            logger.info("Create mapping: " + tpl_resp.isAcknowledged());


        }
        catch(UnknownHostException e) {
            logger.error(e);
        }
        catch(Exception e) {
            logger.error(e);
        }
        finally {
            if(client!=null)
                client.close();
        }
    }

    public void RemoveSet(ArrayList<String> ips) {
    }

    ClusterInfo cluster_info;

    final static String waitforcluster_message = "(4/5) Waiting for cluster ready";
    final static public int default_port = 9200;
    final static public int default_client_port = 9300;

}
