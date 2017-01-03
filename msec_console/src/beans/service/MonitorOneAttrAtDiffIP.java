
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

import beans.dbaccess.OddSecondLevelServiceIPInfo;
import beans.request.MonitorRequest;
import beans.response.MonitorResponse;
import beans.response.OneAttrChart;
import beans.response.OneDayValue;
import ngse.monitor.Monitor;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.ServletConfig;
import ngse.org.Tools;
import org.apache.log4j.Logger;

import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.TreeMap;

/*
1、获得某个service某天的所有attr的列表  ReqService  RespService
2、获得某个IP某天的所有attr的列表   ReqIP   RespIP
3、获得某个service某天某些attr的值      ReqServiceAttr   RespServiceAttr
4、获得某个IP某天某些attr的值                               ReqIPAttr RespIPAttr
5、获得某个service某天某个attr ，分不同IP给出值    ReqAttrIP  RespAttrIP

 */

/*
class  AttrValueAndIP
{
    String IP;
    int[] values;

    public String getIP() {
        return IP;
    }

    public void setIP(String IP) {
        this.IP = IP;
    }

    public int[] getValues() {
        return values;
    }

    public void setValues(int[] values) {
        this.values = values;
    }
}
*/

/**
 * Created by Administrator on 2016/2/23.
 */
public class MonitorOneAttrAtDiffIP extends JsonRPCHandler {

    static public final int PAGE_SIZE = 10;
    static private final  String SESS_KEY_FOR_IP_LIST = "IP_list";
    static private  String monitor_server_ip = "192.168.40.41";
    static private  int monitor_server_port = 3344;



    static private String initMonitorIPAndPort()
    {
        DBUtil util = new DBUtil();
        try
        {
            if (util.getConnection() == null)
            {
                return "connect db failed.";
            }
            String sql =sql = "select ip,port from t_second_level_service_ipinfo where second_level_service_name='monitor' and " +
                    "first_level_service_name='RESERVED' and status='enabled'";


            ArrayList<OddSecondLevelServiceIPInfo>  list = util.findMoreRefResult(sql, null, OddSecondLevelServiceIPInfo.class);
            if (list == null || list.size() < 1)
            {
                return "no db record exists.";
            }
            monitor_server_ip = list.get(0).getIp();
            monitor_server_port = list.get(0).getPort().intValue()+1;//为什么加1，是个很长的故事，源于monitor的开发者将读写分离成两个端口
            return "success";
        }
        catch (Exception e )
        {
            e.printStackTrace();
            return e.getMessage();
        }
        finally {
            util.releaseConn();
        }
    }



    static private ArrayList<String>  getIPListBySvcAndAttr(String svc, String attr)
    {
        Logger logger = Logger.getLogger(MonitorOneAttrAtDiffIP.class);


        Socket sock = new Socket();
        ArrayList<String> ret = new ArrayList<String>();
        try {
            sock.setSoTimeout(2000);
            sock.connect(new InetSocketAddress(monitor_server_ip, monitor_server_port), 3000);

            OutputStream out = sock.getOutputStream();

            //发送请求
            //获取指定svc下的所有ip
            //组包
            Monitor.ReqService.Builder b = Monitor.ReqService.newBuilder();
            b.setServicename(svc);
            Monitor.ReqService reqService = b.build();

            Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
            bb.setService(reqService);
            Monitor.ReqMonitor req = bb.build();


            //发送长度信息
            byte[] lenB = Tools.int2Bytes(req.getSerializedSize()+4);
            out.write(lenB);
            //发送实际的pb请求
            req.writeTo(out);

            logger.info("send request to monitor.");

            //接收应答
            InputStream in = sock.getInputStream();
            byte[] buf = new byte[1024*1024];
            int totalLen = 4;
            int totalReceived = 0;
            while (totalReceived < totalLen) {
                int len = in.read(buf, totalReceived, totalLen - totalReceived);
                if (len <= 0) {
                    return null;
                }
                totalReceived += len;
            }
            totalLen = Tools.bytes2int(buf);

            logger.info("monitor server response length:"+totalLen);

            if (totalLen < 4 || totalLen > buf.length) {
                return null;
            }
            totalLen -= 4;
            totalReceived = 0;
            buf = new byte[totalLen];
            while (totalReceived < totalLen) {
                int len = in.read(buf, totalReceived, totalLen - totalReceived);
                if (len <= 0) {
                    return null;
                }
                totalReceived += len;
            }
            logger.info("received monitor server response successfully.");

            Monitor.RespMonitor monitor = Monitor.RespMonitor.parseFrom(buf);
            if (monitor.getResult() != 0) {
                logger.error("monitor server returns errcode:"+monitor.getResult());
                return null;
            }

            Monitor.RespService resp = monitor.getService();
            for (int i = 0; i < resp.getIpsCount(); ++i) {
                ret.add(resp.getIps(i));
                logger.info("ip#" + i + ":" + resp.getIps(i));

            }

            return ret;

        } catch (Exception e) {
            e.printStackTrace();
            return null;
        } finally {
            if (sock != null && sock.isConnected()) {
                try {
                    sock.close();
                } catch (Exception e) {
                }
            }
        }


    }
    static private ArrayList<OneAttrChart> getAttrValue(String svc, String attr, final ArrayList<String> ipList, String date, String compareDate )
    {

        Logger logger = Logger.getLogger(MonitorOneAttrAtDiffIP.class);
        TreeMap<String, OneAttrChart> ret = new TreeMap<String, OneAttrChart>();

        Socket sock = new Socket();

        logger.info("begin to get attr value from monitor server...");

        try {
            sock.setSoTimeout(5000);
            sock.connect(new InetSocketAddress(monitor_server_ip, monitor_server_port), 3000);
            OutputStream out = sock.getOutputStream();

            //发送请求

            //组包
            Monitor.ReqAttrIP.Builder b = Monitor.ReqAttrIP.newBuilder();
            b.setServicename(svc);
            b.setAttrname(attr);
            b.addDays(new Integer(date).intValue());
            if (compareDate != null && compareDate.length() > 0)
            {
                b.addDays(new Integer(compareDate).intValue());
            }
            for (int i = 0; i < ipList.size(); i++) {
                b.addIps(ipList.get(i));
            }
            Monitor.ReqAttrIP reqAttrIP = b.build();

            Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
            bb.setAttrip(reqAttrIP);
            Monitor.ReqMonitor req = bb.build();


            //发送长度信息
            byte[] lenB = Tools.int2Bytes(req.getSerializedSize()+4);
            out.write(lenB);
            //发送实际的pb请求
            req.writeTo(out);

            logger.info("send request to monitor server.");


            //接收应答
            InputStream in = sock.getInputStream();
            byte[] buf = new byte[1024*1024];
            int totalLen = 4;
            int totalReceived = 0;
            while (totalReceived < totalLen) {
                int len = in.read(buf, totalReceived, totalLen - totalReceived);
                if (len <= 0) {
                    return null;
                }
                totalReceived += len;
            }
            totalLen = Tools.bytes2int(buf);

            logger.info("monitor responds length="+totalLen);

            if (totalLen < 4 || totalLen > buf.length) {
                return null;
            }
            totalLen -= 4;
            totalReceived = 0;
            buf = new byte[totalLen];
            while (totalReceived < totalLen) {
                int len = in.read(buf, totalReceived, totalLen - totalReceived);
                if (len <= 0) {
                    return null;
                }
                totalReceived += len;
            }
            logger.info("received monitor response successfully.");
            Monitor.RespMonitor monitor = Monitor.RespMonitor.parseFrom(buf);
            if (monitor.getResult() != 0) {
                logger.error("monitor returns errcode="+monitor.getResult());
                return null;
            }


            Monitor.RespAttrIP resp = monitor.getAttrip();
            for (int i = 0; i < resp.getDataCount(); ++i) {
                Monitor.AttrIPData data = resp.getData(i);

                //一个图表里可能有两天的数据
                //结果Map中有这张图表，就更新，否则创建一条
                OneAttrChart oneAttrChart = ret.get(data.getIp());
                if (oneAttrChart == null)
                {
                    oneAttrChart = new OneAttrChart(attr, svc, date, compareDate);
                    oneAttrChart.setServer_ip(data.getIp());
                    ret.put(data.getIp(), oneAttrChart);
                }

                int[] value = new int[1440];
                int j;
                for (j = 0; j < data.getValuesCount() && j < value.length; j++) {
                    value[j] = data.getValues(j);
                }
                for (; j < value.length; ++j) {
                    value[j] = 0;
                }

                OneDayValue oneDayValue = null;
                if (oneAttrChart.getValuePerDay()[0].getDate().equals(""+data.getDay()))
                {
                    oneDayValue = oneAttrChart.getValuePerDay()[0];

                }
                else
                {
                    oneDayValue = oneAttrChart.getValuePerDay()[1];
                }

                oneDayValue.setValues(value);

            }


            return new ArrayList<OneAttrChart>(ret.values());


        } catch (Exception e) {
            e.printStackTrace();
            return null;
        } finally {
            if (sock != null && sock.isConnected()) {
                try {
                    sock.close();
                } catch (Exception e) {
                }
            }
        }


    }
    public MonitorResponse exec(MonitorRequest request)
    {
        MonitorResponse response = new MonitorResponse();
        Logger logger = Logger.getLogger(MonitorOneAttrAtDiffIP.class);

        String svc = "";
        String attribute = "";
        String date = "";
        String compareDate = "";
        ArrayList<String> ip_list = null;
        Integer page = 0;
        int page_num = 1;
        String result;

        initMonitorIPAndPort();


        svc = request.getService_name();
        attribute = request.getAttribute();
        if (svc == null || svc.length() < 1 ||
                attribute == null || attribute.length() < 1)
        {
            response.setMessage("svc name and attribute should NOT be both empty!");
            response.setStatus(100);
            return response;
        }

        if (request.getDate() != null && request.getDate().length() >1)
        {
            date = request.getDate();
            ArrayList<String> dates = Tools.splitBySemicolon(date);
            if (dates.size()>0)
            {
                date = dates.get(0);
            }
            else
            {
                date = Tools.nowString("yyyyMMdd");
            }
            if (dates.size()>1) {compareDate = dates.get(1);}
        }
        else
        {
            date = Tools.nowString("yyyyMMdd");
        }
        if (request.getPage() != null)
        {
            page = request.getPage();
        }


        if (ip_list == null)
        {
            //翻页的话，尝试从session里获取ip list
            //这样做的好处是，翻页查看的时间比较长，如果这个时候
            //monitor里有增删属性（增加属性很正常），就不会影响到分页的稳定性
            if (page != 0)
            {
                ip_list = (ArrayList<String>)(getHttpRequest().getSession().getAttribute(SESS_KEY_FOR_IP_LIST));
            }
            if (ip_list == null)
            {
                logger.info("get IP list from session failed, get ip list from monitor server...");
                ip_list = getIPListBySvcAndAttr(svc, attribute);
                if (ip_list == null) {
                    response.setMessage("failed to get IP list from monitor system");
                    response.setStatus(100);
                    return response;
                }
            }
            else
            {
                logger.info("get ip list from session successfully.");
            }
        }
        if (page == 0)
        {
            getHttpRequest().getSession().setAttribute(SESS_KEY_FOR_IP_LIST, ip_list);
            logger.info("write ip list to session successfully.");
        }
        //对ip列表进行分页
        ArrayList<String> ips = null;
        if (ip_list.size() > PAGE_SIZE)
        {
            int i;
            ips = new ArrayList<String>();
            for (i = page * PAGE_SIZE; i < ip_list.size() && i < (page+1) * PAGE_SIZE ;i++)
            {
                ips.add(ip_list.get(i));
            }

            page_num = ip_list.size() / PAGE_SIZE + ( (ip_list.size() % PAGE_SIZE) > 0? 1:0);
            logger.info("page_number:"+page_num);

        }
        else
        {
            ips = new ArrayList<String>();
            for (int i = 0; i < ip_list.size(); i++) {
                ips.add(ip_list.get(i));
            }
            page_num = 1;
        }
        ArrayList<OneAttrChart> attr_values = getAttrValue(svc, attribute, ips, date, compareDate);
        if (attr_values == null)
        {
            response.setMessage("failed to get attr values from monitor system");
            response.setStatus(100);
            return response;
        }
        //画图
        logger.info("begin drawing charts....");

        result = drawCharts(attr_values,  date);
        if (result == null || !result.equals("success"))
        {
            response.setMessage("failed to draw chart"+result);
            response.setStatus(100);
            return response;
        }
        logger.info("draw charts successfully.");

        response.setCharts(attr_values);
        response.setPage_idx(page);
        response.setPage_num(page_num);
        response.setMessage("success");
        response.setStatus(0);
        response.setDate(date);
        response.setCompareDate(compareDate);
        return response;


    }
    private String drawCharts(ArrayList<OneAttrChart> attr,  String date) {
        Logger logger = Logger.getLogger(MonitorOneAttrAtDiffIP.class);

        for (int i = 0; i < attr.size(); i++) {
            String rnd = "."+ (int)(Math.random() * Integer.MAX_VALUE);
            String filename = MonitorBySvcOrIP.getChartDirector() +File.separator+ "attr"+ attr.get(i).getServer_ip()+rnd+".jpg";

            String result = "" ;
            result = Tools.generateFullDayChart(filename,attr.get(i).getValuePerDay(), attr.get(i).getServer_ip());
            if (result == null || !result.equals("success"))
            {
                logger.error("draw chart failed."+result);
                return result;
            }
            logger.info("draw chart:"+filename);
            attr.get(i).setChart_file_name(new File(filename).getName());
        }
        return "success";
    }
}
