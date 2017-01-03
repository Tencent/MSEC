
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

import beans.dbaccess.OddSecondLevelService;
import beans.dbaccess.SecondLevelService;
import beans.dbaccess.SecondLevelServiceIPInfo;
;
import beans.request.IPPortPair;
import ngse.org.AccessZooKeeper;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.Tools;
import org.apache.log4j.Logger;
import org.apache.zookeeper.*;
import org.json.JSONObject;

import java.nio.charset.Charset;
import java.util.*;
import java.util.concurrent.CountDownLatch;


/**
 * Created by Administrator on 2016/3/2.
 */
public class LoadBalance {

    static public String startLBSrv() {

        /*
        String[] cmd = {"/usr/local/zookeeper/bin/zkServer.sh", "start"};
        StringBuffer sb = new StringBuffer();
        int v = Tools.runCommand(cmd, sb, true);
        */

        return "success";
    }

    static public String stopLBSrv() {
        String[] cmd = {"/usr/local/zookeeper/bin/zkServer.sh", "stop"};
        StringBuffer sb = new StringBuffer();
        int v = Tools.runCommand(cmd, sb, true);

        return "success";
    }

    //写一个服务的IP信息到LB
    static public String writeOneServiceConfigInfo(AccessZooKeeper azk, String svcname,boolean isStandard,
                                                   ArrayList<IPPortPair> iplist) throws Exception
    {
        Logger logger = Logger.getLogger(LoadBalance.class);

        /*
        logger.info("generate IP->[port list] map...");
        Map<String, String>  map = new HashMap<String, String>();// ip -> ports
        for (int i = 0; i < iplist.size(); i++) {
            IPPortPair pair = iplist.get(i);

            String ports = map.get(pair.getIp());
            if (ports == null)
            {
                ports = "" + pair.getPort() ;
                map.put(pair.getIp(), ports);
            }
            else
            {
                ports = ports + ", " + pair.getPort() ;
                map.remove(pair.getIp());
                map.put(pair.getIp(), ports);
            }
        }
        StringBuffer data = new StringBuffer();
        data.append("{ \"IPInfo\":["); //ipinfo是一个数组
        int eleIndex = 0;
        for (Map.Entry<String, String> entry : map.entrySet()) {
            if (eleIndex > 0)
            {
                data.append(","); //与前面的数组元素分割起来
            }
            //数组的每一个元素，是一个对象，包括w t ip  ports四个字段
           data.append("{");


           data.append("\"w\":100, \"t\":\"all\", \"IP\":\"");
            data.append(entry.getKey());
            //ports也是一个数组
            data.append("\",\"ports\":[");
            data.append(entry.getValue());
            data.append("]");

            data.append("}");//一个对象，或者说一个数组元素结束

            eleIndex++;

        }
        data.append("]}");
        */
        StringBuffer data = new StringBuffer();
        if (isStandard)
        {
            data.append("{ \"Policy\":\"standard\"");
        }
        else
        {
            data.append("{ \"Policy\":\"odd\"");
        }
        data.append(", \"IPInfo\":["); //ipinfo是一个数组
        int eleIndex = 0;
        for (int i = 0; i < iplist.size();++i)
        {
            if (eleIndex > 0)
            {
                data.append(","); //与前面的数组元素分割起来
            }
            //数组的每一个元素，是一个对象，包括w t ip  ports四个字段
            data.append("{");
            IPPortPair oneEle = iplist.get(i);
            String comm_proto = oneEle.getComm_proto();
            if (comm_proto.equals("tcp and udp"))
            {
                comm_proto = "all";
            }
            data.append("\"w\":100, \"t\":\""+comm_proto+"\", \"IP\":\"");
            data.append(oneEle.getIp());
            //ports也是一个数组
            data.append("\",\"ports\":[");
            data.append(oneEle.getPort());
            data.append("]");

            data.append("}");//一个对象，或者说一个数组元素结束

            eleIndex++;

        }
        data.append("]}");


        String path = "/nameservice/" + svcname;

        logger.info("write LB server: path="+path);
        logger.info("data="+data.toString());

        String result = azk.write(path, data.toString().getBytes());
        if (result == null || !result.equals("success"))
        {
            logger.error("LB write failed:"+result);
            return result;
        }

        return "success";
    }

    //读一个服务的IP信息到LB
    static public byte[] readOneServiceConfigInfo(AccessZooKeeper azk, String svcname) throws Exception
    {
        String path = "/nameservice/"+svcname;
        byte[] ret =  azk.read(path);

        return ret;
    }
    //读取一个服务指定IP的负载信息
    static public int  readOneServiceLoadInfo(AccessZooKeeper azk, String svcname, String ip) throws Exception
    {
        String path = "/loadreport/"+ip;

        Logger logger = Logger.getLogger(LoadBalance.class);

        byte[] data = azk.read(path);
        if (data == null||data.length < 10)//不存在
        {
            logger.error("load information NOT exist:"+path);
            return -1;
        }
        String jsonStr = new String(data, Charset.forName("UTF-8"));

        logger.info("read ip load info from LB:path="+path);
        logger.info("data="+jsonStr);

        JSONObject obj = new JSONObject(jsonStr);
        int  cpu = obj.getInt("cpu");
        //cpu = cpu / 100;
        return cpu;
    }


    //从数据库中获取所有的服务
    static public ArrayList<SecondLevelService> getAllService( DBUtil util)
    {

        ArrayList<SecondLevelService> serviceList ;

        String sql = "select first_level_service_name, second_level_service_name,type from t_second_level_service";
        List<Object> params = new ArrayList<Object>();

        try {
            serviceList = util.findMoreRefResult(sql, params, SecondLevelService.class);
            return serviceList;

        }
        catch (Exception e)
        {
            e.printStackTrace();

           return null;
        }

    }



    //从数据库中获取一个服务的所有IP信息
    static public ArrayList<IPPortPair> getIPPortInfoByServiceName(String flsn, String slsn,  DBUtil util)
    {

        ArrayList<IPPortPair> ipList ;
        List<Object> params = new ArrayList<Object>();
        String sql = "";


        sql = "select ip,port,status,comm_proto from t_second_level_service_ipinfo where " +
                "second_level_service_name=? and first_level_service_name=? and status='enabled'";

        params.add(slsn);
        params.add(flsn);

        try {
            ipList = util.findMoreRefResult(sql, params, IPPortPair.class);
            return ipList;
        } catch (Exception e) {
            e.printStackTrace();

            return null;
        }

    }

    //建立从IP到服务列表的映射
    static private void   geneIP2Svcs(String svc, ArrayList<IPPortPair> ips,
                                      HashMap<String, HashSet<String>> ip2Svcs )
    {

        for (int j = 0; j < ips.size(); j++) {
            String ip = ips.get(j).getIp();
            if (ip2Svcs.get(ip) == null)
            {
                HashSet<String> value = new HashSet<String>();
                value.add(svc);
                ip2Svcs.put(ip,value );
            }
            else
            {
                HashSet<String> value = ip2Svcs.get(ip);
                value.add(svc);
            }
        }
        return;
    }

    //把所有服务的配置信息写入LB
    static public String writeServiceConfigInfo(AccessZooKeeper azk) throws Exception
    {
        DBUtil util = new DBUtil();
        Logger logger = Logger.getLogger(LoadBalance.class);

        if (util.getConnection() == null)
        {
            return "db connect failed";
        }
        logger.info("write IP information of ALL services into LB...");
        try {
            //删除掉整个配置信息
            String path = "/nameservice";
            azk.deleteRecursive(path);

            logger.info("delete whole tree:"+path);

            //获得所有服务
            ArrayList<SecondLevelService> services = getAllService(util);
            if (services == null) {
                return "get standard service list from db failed";
            }
            logger.info("getAllService() OK, service number:"+services.size());

            //对每个服务获取下属的IP
            for (int i = 0; i < services.size(); i++) {
                SecondLevelService svc = services.get(i);
                ArrayList<IPPortPair> ips = getIPPortInfoByServiceName(svc.getFirst_level_service_name(),
                        svc.getSecond_level_service_name(), util);
                if (ips == null) {
                    return "get ip list from db failed";
                }
                logger.info("get ip for "+svc.getSecond_level_service_name()+" OK, ip number"+ips.size());
                //写到 LB系统里
                String result = writeOneServiceConfigInfo(azk,
                        svc.getFirst_level_service_name() + "/" + svc.getSecond_level_service_name(),
                        svc.getType().equals("standard"),
                        ips);
                if (result == null || !result.equals("success"))
                {
                    logger.error("write service IP information into LB failed!"+result);
                    return result;
                }
                logger.info("write to LB ok");


            }




            return "success";

        }
        finally {
            util.releaseConn();
        }
    }
    //以IP为key的配置信息写入LB，whiteNameList为需要写的白名单IP列表，可以为null
    static public String writeIPConfigInfo(AccessZooKeeper azk, ArrayList<String> whiteNameList) throws Exception
    {
        Logger logger = Logger.getLogger(LoadBalance.class);
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            return "db connect failed";
        }
        logger.info("write IP key info into LB ...");
        HashMap<String, HashSet<String>> ip2Svcs = new HashMap<String, HashSet<String>>();
        try {
            //获得所有服务
            ArrayList<SecondLevelService> services = getAllService(util);
            if (services == null) {
                logger.error("get all service failed");
                return "get service list from db failed";
            }
            //对每个服务获取下属的IP
            for (int i = 0; i < services.size(); i++) {
                SecondLevelService svc = services.get(i);
                ArrayList<IPPortPair> ips = getIPPortInfoByServiceName(svc.getFirst_level_service_name(),
                        svc.getSecond_level_service_name(), util);
                if (ips == null) {
                    logger.error("get ip list for "+svc.getSecond_level_service_name()+" failed.");
                    return "get ip list from db failed";
                }
                logger.info("get ip list for "+svc.getSecond_level_service_name()+" successfully.ip number"+ips.size());

                //生成IP为key、servicename列表为value的反向映射关系
                geneIP2Svcs(svc.getFirst_level_service_name() + "." + svc.getSecond_level_service_name(),
                        ips, ip2Svcs);

            }
            logger.info("gene IP->service map successfully, map size:"+ip2Svcs.size());


            if (whiteNameList == null)
            {
                //删除掉整个ip配置信息
                String path = "/nodeservices";
                azk.deleteRecursive(path);

                logger.info("delete whole tree:"+path);


            }
            else
            {
                //删除掉白名单中的ip配置信息
                for (int i = 0; i < whiteNameList.size() ; i++) {
                    String path = "/nodeservices/"+whiteNameList.get(i);
                    azk.deleteRecursive(path);
                    logger.info("delete node:"+path);

                }

            }

            //往LB写入ip为key的数据
            for (Map.Entry<String, HashSet<String>> entry : ip2Svcs.entrySet()) {

                if (whiteNameList != null)//如果只更新列表中的指定的IP列表
                {
                    if (whiteNameList.indexOf(entry.getKey()) < 0)
                    {
                        continue;
                    }
                }

                String path = "/nodeservices/"+entry.getKey();
                String data = "{\"services\":[";
                Iterator<String> it = entry.getValue().iterator();
                int eleIndex =0;
                while( it.hasNext())
                {
                    if (eleIndex > 0)
                    {
                        data += ","; //与前一个元素的分割
                    }
                    data += "\""+ it.next() +"\"";

                    eleIndex++;
                }
                data += "]}";

                logger.info("write LB, path="+path);
                logger.info("data="+data);

                String result = azk.write(path, data.getBytes());
                if (result == null || !result.equals("success"))
                {
                    logger.error("write LB failed:"+result);
                    return result;
                }
            }
            return "success";

        }
        finally {
            util.releaseConn();
        }
    }





}
