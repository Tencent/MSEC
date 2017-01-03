
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
import beans.dbaccess.SecondLevelServiceIPInfo;
import beans.request.MonitorRequest;
import beans.response.MonitorResponse;
import beans.response.OneAttrChart;
import beans.response.OneAttrDaysChart;
import com.google.protobuf.ByteString;

import ngse.monitor.Monitor;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.ServletConfig;
import ngse.org.Tools;
import beans.response.OneDayValue;
import org.apache.log4j.Logger;
import sun.rmi.runtime.Log;


import javax.tools.Tool;
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.management.MonitorInfo;
import java.lang.reflect.Array;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.TreeMap;
import java.util.concurrent.ExecutionException;
import java.util.regex.Pattern;

/*
class AttrValueAndName
{
    String name;
    int[] value;

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public int[] getValue() {
        return value;
    }

    public void setValue(int[] value) {
        this.value = value;
    }
}
*/

/*
1、获得某个service某天的所有attr的列表  ReqService  RespService
2、获得某个IP某天的所有attr的列表   ReqIP   RespIP
3、获得某个service某天某些attr的值      ReqServiceAttr   RespServiceAttr
4、获得某个IP某天某些attr的值                               ReqIPAttr RespIPAttr
5、获得某个service某天某个attr ，分不同IP给出值    ReqAttrIP  RespAttrIP

 */

/**
 * Created by Administrator on 2016/2/23.
 */
public class MonitorBySvcOrIP extends JsonRPCHandler {

    static public final int PAGE_SIZE = 10;
    static private final  String SESS_KEY_FOR_ATTR_LIST = "attr_list";
    static private  String monitor_server_ip = "192.168.40.41";
    static private  int monitor_server_port = 3344;

    static public  String getChartDirector()
    {
        return ServletConfig.fileServerRootDir + File.separator + "tmp";
    }

    static public void initMonitorIPAndPort() throws Exception
    {
        DBUtil util = new DBUtil();
        Logger logger = Logger.getLogger(MonitorBySvcOrIP.class);
        try
        {
            if (util.getConnection() == null)
            {
                Exception e = new Exception("connect db failed.");
                throw e ;
            }
            String sql =sql = "select ip,port from t_second_level_service_ipinfo where second_level_service_name='monitor' and " +
                    "first_level_service_name='RESERVED'  and status='enabled'";


            ArrayList<OddSecondLevelServiceIPInfo>  list = util.findMoreRefResult(sql, null, OddSecondLevelServiceIPInfo.class);
            if (list == null || list.size() < 1)
            {
                throw  new Exception( "no db record exists.");
            }
            monitor_server_ip = list.get(0).getIp();
            monitor_server_port = list.get(0).getPort().intValue()+1;//为什么加1，是个很长的故事，源于monitor的开发者将读写分离成两个端口
            logger.info(String.format("monitor server:%s:%d", monitor_server_ip, monitor_server_port) );
        }
        finally {
            util.releaseConn();
        }
    }



    static public ArrayList<String>  getAttrListBySvcAndIP(String svc, String IP,
                                                            String date, String compareDate,
                                                           String attr_re /*正则表达式过滤attr*/) throws Exception
    {

        Logger logger = Logger.getLogger(MonitorBySvcOrIP.class);

        logger.info("begin to get attr list by svc (or IP)...");
        Pattern pattern = null;
        if (attr_re.length()>1)
        {
            pattern = Pattern.compile(attr_re, Pattern.CASE_INSENSITIVE);
        }

        ArrayList<String> ret = new ArrayList<String>();
        Socket sock = new Socket();
        try {
            sock.setSoTimeout(5000);
            sock.connect(new InetSocketAddress(monitor_server_ip, monitor_server_port), 3000);

            OutputStream out = sock.getOutputStream();

            //发送请求
            if (IP.length() > 1) { //获取指定svc下的某IP下的所有attr
                Monitor.ReqIP.Builder b = Monitor.ReqIP.newBuilder();
                b.setIp(IP);

                b.addDays(new Integer(date).intValue());
                if (compareDate != null && compareDate.length() > 1)
                {
                    b.addDays(new Integer(date).intValue());
                }
                Monitor.ReqIP reqIP = b.build();

                Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
                bb.setIp(reqIP);
                Monitor.ReqMonitor req = bb.build();
                //发送长度信息
                byte[] len = Tools.int2Bytes(req.getSerializedSize()+4);
                out.write(len);
                //发送实际的pb请求
                req.writeTo(out);
            }
            else{ //获取指定svc下的所有attr
                //组包
                Monitor.ReqService.Builder b = Monitor.ReqService.newBuilder();
                b.setServicename(svc);
                b.addDays(new Integer(date).intValue());
                if (compareDate != null && compareDate.length() > 1)
                {
                    b.addDays(new Integer(date).intValue());
                }
                Monitor.ReqService reqService = b.build();

                Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
                bb.setService(reqService);
                Monitor.ReqMonitor req = bb.build();


                //发送长度信息
                int reqLen = req.getSerializedSize();
                byte[] len = Tools.int2Bytes(reqLen+4);
                out.write(len);
                //发送实际的pb请求
                req.writeTo(out);
            }
            logger.info("send request to monitor server");

            //接收应答
            InputStream in = sock.getInputStream();
            byte[] buf = new byte[1024*1024];
            int totalLen = 4;
            int totalReceived = 0;
            while (totalReceived < totalLen)
            {
                int len = in.read(buf, totalReceived, totalLen-totalReceived);
                if (len <= 0)
                {
                    logger.error(String.format("read() failed:%d", len));
                    return null;
                }
                totalReceived += len;
            }
            totalLen = Tools.bytes2int(buf);
            logger.info("monitor server respond, length:"+totalLen);


            if (totalLen < 4 || totalLen > buf.length)
            {
                logger.error(String.format("totalLen invalid:%d", totalLen));
                return null;
            }
            totalLen -=4;

            totalReceived = 0;
            buf = new byte[totalLen];
            while (totalReceived < totalLen)
            {
                int len = in.read(buf, totalReceived, totalLen-totalReceived);
                if (len <= 0)
                {
                    logger.error(String.format("read() response body failed:%d", len));
                    return null;
                }
                totalReceived += len;
            }
            logger.info("received server response successfully.");
            Monitor.RespMonitor monitor = Monitor.RespMonitor.parseFrom(buf);
            if (monitor.getResult() != 0)
            {
                logger.error("monitor server returns error, result code="+monitor.getResult());
                return null;
            }
            if (IP.length() > 1) //获取指定svc下的某IP下的所有attr
            {

                Monitor.RespIP resp = monitor.getIp();
                for (int i = 0; i < resp.getDataCount(); ++i)
                {
                    Monitor.IPData data = resp.getData(i);
                    if (data.getServicename().equals(svc)) { // svc筛选一下
                        for (int j = 0; j < data.getAttrnamesCount(); ++j) {
                            if (pattern != null)
                            {
                                if (  !pattern.matcher(data.getAttrnames(j)).find())
                                {
                                    continue;
                                }
                            }
                            ret.add(data.getAttrnames(j));
                            logger.info("attr name:"+data.getAttrnames(j));
                        }
                    }
                }

            }
            else
            {
                Monitor.RespService resp = monitor.getService();
                for (int i = 0; i < resp.getAttrnamesCount(); ++i)
                {
                    if (pattern != null)
                    {
                        if (!pattern.matcher(resp.getAttrnames(i)).find())
                        {
                            continue;
                        }
                    }
                   ret.add(resp.getAttrnames(i));
                    logger.info("attr name:"+resp.getAttrnames(i));
                }
            }
            logger.info("attr number:"+ret.size());
            return ret;

        }

        finally {
            if (sock != null && sock.isConnected())
            {
                try {sock.close();} catch (Exception e) {}
            }
        }
    }

    static public ArrayList<OneAttrChart> getAttrValue(String svc, String IP,
                                                        final ArrayList<String> attrList,
                                                        String date, String compareDate ) throws Exception
    {

        Socket sock = new Socket();
        Logger logger = Logger.getLogger(MonitorBySvcOrIP.class);

        logger.info("begin to get attr value...");

        TreeMap<String, OneAttrChart> ret = new   TreeMap<String, OneAttrChart> ();
        try {
            sock.setSoTimeout(5000);
            sock.connect(new InetSocketAddress(monitor_server_ip, monitor_server_port), 3000);
            OutputStream out = sock.getOutputStream();

            //发送请求
            if (IP.length() > 1) { //获取指定svc下的某IP下的指定attr的值
                Monitor.ReqIPAttr.Builder b = Monitor.ReqIPAttr.newBuilder();

                //该svc下的attr
                Monitor.IPData.Builder ipdataBuild = Monitor.IPData.newBuilder();
                ipdataBuild.setServicename(svc);
                for (int i = 0; i < attrList.size(); i++) {
                    ipdataBuild.addAttrnames(attrList.get(i));

                }

                //一个IP下有多个svc，指定该svc
                b.addAttrs(ipdataBuild.build());
                b.setIp(IP);
                b.addDays(new Integer(date).intValue());
                if (compareDate != null && compareDate.length() > 1)
                {
                    b.addDays(new Integer(compareDate).intValue());
                }
                Monitor.ReqIPAttr reqIPAttr = b.build();

                Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
                bb.setIpattr(reqIPAttr);
                Monitor.ReqMonitor req = bb.build();

                //发送长度信息
                byte[] len = Tools.int2Bytes(req.getSerializedSize()+4);
                out.write(len);
                //发送实际的pb请求
                req.writeTo(out);
            }
            else{ //获取指定svc下的所有attr
                //组包
                Monitor.ReqServiceAttr.Builder b = Monitor.ReqServiceAttr.newBuilder();
                b.setServicename(svc);
                b.addDays( new Integer(date).intValue());
                if (compareDate != null && compareDate.length() > 1)
                {
                    b.addDays(new Integer(compareDate).intValue());
                }

                for (int i = 0; i < attrList.size(); i++) {
                    b.addAttrnames(attrList.get(i));
                }
                Monitor.ReqServiceAttr reqServiceAttr = b.build();

                Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
                bb.setServiceattr(reqServiceAttr);
                Monitor.ReqMonitor req = bb.build();


                //发送长度信息
                byte[] len = Tools.int2Bytes(req.getSerializedSize()+4);
                out.write(len);
                //发送实际的pb请求
                req.writeTo(out);
            }
            logger.info("send request to monitor server");

            //接收应答
            InputStream in = sock.getInputStream();
            byte[] buf = new byte[1024*1024];
            int totalLen = 4;
            int totalReceived = 0;
            while (totalReceived < totalLen)
            {
                int len = in.read(buf, totalReceived, totalLen-totalReceived);
                if (len <= 0)
                {
                    logger.error(String.format("read len field failed:%d", len));
                    return null;
                }
                totalReceived += len;
            }

            totalLen = Tools.bytes2int(buf);

            logger.info("monitor server response size:"+totalLen);


            if (totalLen < 4 || totalLen > buf.length)
            {
                logger.error(String.format("totalLen invalid:%d", totalLen));
                return null;
            }
            totalLen -=4;
            totalReceived = 0;
            buf = new byte[totalLen];
            while (totalReceived < totalLen)
            {
                int len = in.read(buf, totalReceived, totalLen-totalReceived);
                if (len <= 0)
                {
                    logger.error(String.format("read response body failed:%d", len));
                    return null;
                }
                totalReceived += len;
            }

            Monitor.RespMonitor monitor = Monitor.RespMonitor.parseFrom(buf);
            if (monitor.getResult() != 0)
            {
                logger.error("monitor returns error code:"+monitor.getResult());
                return null;
            }
            if (IP.length() > 1) //获取指定svc下的某IP下的所有attr
            {

                Monitor.RespIPAttr resp = monitor.getIpattr();
                for (int i = 0; i < resp.getDataCount(); ++i)
                {
                    Monitor.IPAttrData data = resp.getData(i);
                    if (data.getServicename().equals(svc)) { // svc筛选一下

                        //一个图表里可能有两天的数据
                        //结果Map中有这张图表，就更新，否则创建一条
                        OneAttrChart oneAttrChart = ret.get(data.getAttrname());
                        if (oneAttrChart == null)
                        {
                            oneAttrChart = new OneAttrChart(data.getAttrname(),
                                    svc,
                                    date,
                                    compareDate);
                            oneAttrChart.setServer_ip(IP);

                            ret.put(data.getAttrname(), oneAttrChart);
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
                        oneDayValue.setDate("" + data.getDay());
                        logger.info("get one day values for " + data.getDay());

                    }
                }

            }
            else
            {
                Monitor.RespServiceAttr resp = monitor.getServiceattr();
                for (int i = 0; i < resp.getDataCount(); ++i)
                {

                    Monitor.AttrData data = resp.getData(i);
                    //一个图表里可能有两天的数据
                    //结果Map中有这张图表，就更新，否则创建一条
                    OneAttrChart oneAttrChart = ret.get(data.getAttrname());
                    if (oneAttrChart == null)
                    {
                        oneAttrChart = new OneAttrChart(
                                data.getAttrname(),
                                svc,
                                date, compareDate);


                        ret.put(data.getAttrname(), oneAttrChart);
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
                    oneDayValue.setDate("" + data.getDay());

                    logger.info("get one day values for " + data.getDay());

                }
            }
            return new ArrayList<OneAttrChart>(ret.values());



        }
        finally {
            if (sock != null && sock.isConnected())
            {
                try {sock.close();} catch (Exception e) {}
            }
        }
    }

    static public ArrayList<OneDayValue> getAttrDaysValue(String svc, String attr, String IP,
                                                       final ArrayList<String> dateList ) throws Exception
    {

        Socket sock = new Socket();
        Logger logger = Logger.getLogger(MonitorBySvcOrIP.class);

        logger.info("begin to get attr value...");
        logger.info(dateList);

        TreeMap<String, OneDayValue> result_map = new TreeMap<>();
        try {
            sock.setSoTimeout(5000);
            sock.connect(new InetSocketAddress(monitor_server_ip, monitor_server_port), 3000);
            OutputStream out = sock.getOutputStream();

            //发送请求
            if (IP!=null &&  IP.length() > 1) { //获取指定attr指定IP下多天数据
                Monitor.ReqAttrIP.Builder b = Monitor.ReqAttrIP.newBuilder();
                b.setServicename(svc);
                b.setAttrname(attr);
                b.addIps(IP);
                //一个IP下有多个svc，指定该svc
                for(String date : dateList) {
                    b.addDays(new Integer(date).intValue());
                }
                Monitor.ReqAttrIP reqAttrIP = b.build();

                Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
                bb.setAttrip(reqAttrIP);
                Monitor.ReqMonitor req = bb.build();

                //发送长度信息
                byte[] len = Tools.int2Bytes(req.getSerializedSize()+4);
                out.write(len);
                //发送实际的pb请求
                req.writeTo(out);
            }
            else{ //获取指定attr多天数据
                //组包
                Monitor.ReqServiceAttr.Builder b = Monitor.ReqServiceAttr.newBuilder();
                b.setServicename(svc);
                for(String date : dateList) {
                    b.addDays(new Integer(date).intValue());
                }
                b.addAttrnames(attr);
                Monitor.ReqServiceAttr reqServiceAttr = b.build();

                Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
                bb.setServiceattr(reqServiceAttr);
                Monitor.ReqMonitor req = bb.build();


                //发送长度信息
                byte[] len = Tools.int2Bytes(req.getSerializedSize()+4);
                out.write(len);
                //发送实际的pb请求
                req.writeTo(out);
            }
            logger.info("send request to monitor server");

            //接收应答
            InputStream in = sock.getInputStream();
            byte[] buf = new byte[1024*1024];
            int totalLen = 4;
            int totalReceived = 0;
            while (totalReceived < totalLen)
            {
                int len = in.read(buf, totalReceived, totalLen-totalReceived);
                if (len <= 0)
                {
                    logger.error(String.format("read len field failed:%d", len));
                    return null;
                }
                totalReceived += len;
            }

            totalLen = Tools.bytes2int(buf);

            logger.info("monitor server response size:"+totalLen);


            if (totalLen < 4 || totalLen > buf.length)
            {
                logger.error(String.format("totalLen invalid:%d", totalLen));
                return null;
            }
            totalLen -=4;
            totalReceived = 0;
            buf = new byte[totalLen];
            while (totalReceived < totalLen)
            {
                int len = in.read(buf, totalReceived, totalLen-totalReceived);
                if (len <= 0)
                {
                    logger.error(String.format("read response body failed:%d", len));
                    return null;
                }
                totalReceived += len;
            }

            Monitor.RespMonitor monitor = Monitor.RespMonitor.parseFrom(buf);
            if (monitor.getResult() != 0)
            {
                logger.error("monitor returns error code:"+monitor.getResult());
                return null;
            }
            if (IP!=null && IP.length() > 1) //获取指定svc下的某IP下的所有attr
            {

                Monitor.RespAttrIP resp = monitor.getAttrip();
                for (int i = 0; i < resp.getDataCount(); ++i)
                {
                    Monitor.AttrIPData data = resp.getData(i);
                    int[] value = new int[1440];
                    int j;
                    for (j = 0; j < data.getValuesCount() && j < value.length; j++) {
                        value[j] = data.getValues(j);
                    }
                    for (; j < value.length; ++j) {
                        value[j] = 0;
                    }
                    OneDayValue oneDayValue = new OneDayValue("" + data.getDay(),value);
                    logger.info("get one day values for " + data.getDay());
                    result_map.put("" + data.getDay(), oneDayValue);
                }
            }
            else
            {
                Monitor.RespServiceAttr resp = monitor.getServiceattr();
                for (int i = 0; i < resp.getDataCount(); ++i)
                {
                    Monitor.AttrData data = resp.getData(i);

                    int[] value = new int[1440];
                    int j;
                    for (j = 0; j < data.getValuesCount() && j < value.length; j++) {
                        value[j] = data.getValues(j);
                    }
                    for (; j < value.length; ++j) {
                        value[j] = 0;
                    }
                    OneDayValue oneDayValue = new OneDayValue("" + data.getDay(),value);
                    logger.info("get one day values for " + data.getDay());
                    result_map.put("" + data.getDay(), oneDayValue);
                }
            }
            for(String date : dateList) {
                if(!result_map.containsKey(date)) {
                    int[] value = new int[1440];    //default to 0
                    OneDayValue oneDayValue = new OneDayValue(date, value);
                    result_map.put(date,oneDayValue);
                }
            }
            return new ArrayList<>(result_map.values());
        }
        finally {
            if (sock != null && sock.isConnected())
            {
                try {sock.close();} catch (Exception e) {}
            }
        }
    }

    public MonitorResponse exec(MonitorRequest request)
    {
        MonitorResponse response = new MonitorResponse();
        Logger logger = Logger.getLogger(MonitorBySvcOrIP.class);

        String svc = "";
        String server_ip = "";
        String date = "";
        String compareDate = "";
        String attr_re = "";
        ArrayList<String> attr_list = null;
        Integer page = 0;
        int page_num = 1;
        String result;

        result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }

        try {

            initMonitorIPAndPort();


            svc = request.getService_name();

            if (request.getDate() != null && request.getDate().length() > 0) {
                date = request.getDate();
                ArrayList<String> dates = Tools.splitBySemicolon(date);
                if (dates.size() > 0) {
                    date = dates.get(0);
                } else {
                    date = Tools.nowString("yyyyMMdd");
                }
                if (dates.size() > 1) {
                    compareDate = dates.get(1);
                }
            } else {
                date = Tools.nowString("yyyyMMdd");
                logger.info("date is today:" + date);
            }
            if (request.getServer_ip() != null && request.getServer_ip().length() > 1) {
                server_ip = request.getServer_ip();
            }
            if (request.getAttribute() != null && request.getAttribute().length() > 1) {
                if (request.getAttribute().charAt(0) == '^') //正则表达式
                {
                    attr_re = request.getAttribute();
                    attr_list = null;
                }
                else {
                    attr_re = "";
                    attr_list = Tools.splitBySemicolon(request.getAttribute());
                }
            }
            if (request.getPage() != null) {
                page = request.getPage();
            }

            if (svc.length() == 0 && server_ip.length() == 0) {
                response.setMessage("svc name and server ip should NOT be both empty!");
                response.setStatus(100);
                return response;
            }
            if (attr_list == null) {
                //翻页的话，尝试从session里获取attr list
                //这样做的好处是，翻页查看的时间比较长，如果这个时候
                //monitor里有增删属性（增加属性很正常），就不会影响到分页的稳定性
                if (page != 0) {
                    @SuppressWarnings("unchecked")
                    ArrayList<String> tmp_list = (ArrayList<String>) (getHttpRequest().getSession().getAttribute(SESS_KEY_FOR_ATTR_LIST));
                    attr_list = tmp_list;
                }
                if (attr_list == null) {
                    attr_list = getAttrListBySvcAndIP(svc, server_ip, date, compareDate, attr_re);
                    if (attr_list == null) {
                        response.setMessage("failed to get attr list from monitor system");
                        response.setStatus(100);
                        return response;
                    }
                    logger.info("get attr list from monitor server successfully.");
                } else {
                    logger.info("get attr list from session successfully.");
                }
            }
            if (page == 0) {
                getHttpRequest().getSession().setAttribute(SESS_KEY_FOR_ATTR_LIST, attr_list);
                logger.info("save attr list into session successfully.");
            }
            //对属性列表进行分页
            ArrayList<String> attrs = null;
            if (attr_list.size() > PAGE_SIZE) {
                int i;
                attrs = new ArrayList<String>();
                for (i = page * PAGE_SIZE; i < attr_list.size() && i < (page + 1) * PAGE_SIZE; i++) {
                    attrs.add(attr_list.get(i));
                }

                page_num = attr_list.size() / PAGE_SIZE + ((attr_list.size() % PAGE_SIZE) > 0 ? 1 : 0);

                logger.info("seperate pages, page number:" + page_num);

            } else {
                attrs = new ArrayList<String>();
                for (int i = 0; i < attr_list.size(); i++) {
                    attrs.add(attr_list.get(i));
                }
                page_num = 1;
            }
            ArrayList<OneAttrChart> attr_values = getAttrValue(svc, server_ip, attrs, date, compareDate);
            if (attr_values == null) {
                response.setMessage("failed to get attr values from monitor system");
                response.setStatus(100);
                return response;
            }

            logger.info("get attr value successfully.");
            //画图
            drawCharts(attr_values, date);

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
        catch (Exception e) {
            response.setStatus(100);
            response.setMessage(e.getMessage());
            return response;
        }
    }

    public static  void drawCharts(ArrayList<OneAttrChart> attr,  String date) throws Exception{
        Logger logger = Logger.getLogger(MonitorBySvcOrIP.class);
        for (int i = 0; i < attr.size(); i++) {
            String rnd = "."+ (int)(Math.random() * Integer.MAX_VALUE);
           // String filename = getChartDirector() +File.separator+ attr.get(i).getAttribute()+rnd+".jpg";
            String strMD5 = Tools.md5(attr.get(i).getAttribute());
            String filename = getChartDirector() +File.separator+strMD5 +rnd+".jpg";
            String result = Tools.generateFullDayChart(filename, attr.get(i).getValuePerDay(), attr.get(i).getAttribute());
            if (result == null || !result.equals("success"))
            {
                logger.error("draw chart failed. result:"+result);
                throw new Exception( result);
            }
            logger.info("draw chart successfully, filename:"+filename);

            attr.get(i).setChart_file_name(new File(filename).getName());
        }
        return;
    }

    public static void drawDaysChart(OneAttrDaysChart chart, ArrayList<OneDayValue> values, String title, int duration ) throws Exception{
        Logger logger = Logger.getLogger(MonitorBySvcOrIP.class);
        String rnd = "."+ (int)(Math.random() * Integer.MAX_VALUE);
        // String filename = getChartDirector() +File.separator+ attr.get(i).getAttribute()+rnd+".jpg";
        String strMD5 = Tools.md5(chart.getAttribute());
        String filename = getChartDirector() +File.separator+strMD5 +rnd+".jpg";
        String result = Tools.generateDaysChart(filename, values, chart, title, duration);
        if (result == null || !result.equals("success"))
        {
            logger.error("draw chart failed. result:"+result);
            throw new Exception( result);
        }
        logger.info("draw chart successfully, filename:" + filename + "|" + chart.getSum() +"|"+ chart.getMax()+"|"+ chart.getMin());
        chart.setChart_file_name(new File(filename).getName());
    }
}
