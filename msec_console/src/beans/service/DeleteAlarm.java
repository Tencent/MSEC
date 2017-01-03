
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
import beans.request.Alarm;
import beans.response.AlarmResponse;
import beans.response.OneAlarm;
import ngse.monitor.Monitor;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.Tools;
import org.apache.log4j.Logger;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;

/**
 * Created by Administrator on 2016/4/27.
 */
public class DeleteAlarm extends JsonRPCHandler {
    static private  String monitor_server_ip = "192.168.40.41";
    static private  int monitor_server_port = 3344;


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
            String sql =sql = "select ip,port from t_second_level_service_ipinfo where second_level_service_name='monitor' " +
                    "and first_level_service_name='RESERVED' and status='enabled'";


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

    static public void  delAlarm(String svc, String attr, String date, long lastOccurTime, int type ) throws Exception
    {

        Logger logger = Logger.getLogger(DeleteAlarm.class);


        Socket sock = new Socket();
        ArrayList<String> ret = new ArrayList<String>();
        try {
            sock.setSoTimeout(2000);
            sock.connect(new InetSocketAddress(monitor_server_ip, monitor_server_port), 3000);

            OutputStream out = sock.getOutputStream();

            //发送请求
            //获取指定svc下的所有ip
            //组包
            Monitor.ReqDelAlarm.Builder b = Monitor.ReqDelAlarm.newBuilder();
            b.setServicename(svc);
            b.setAttrname(attr);
            b.setDay(new Integer(date));
            b.setTime( new Long(lastOccurTime).intValue());
            b.setType(Monitor.AlarmType.valueOf(type));
            Monitor.ReqDelAlarm reqDelAlarm = b.build();

            Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
            bb.setDelalarm(reqDelAlarm);
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
                    throw new Exception("monitor server应答无效.");
                }
                totalReceived += len;
            }
            totalLen = Tools.bytes2int(buf);

            logger.info("monitor server response length:"+totalLen);

            if (totalLen < 4 || totalLen > buf.length) {
                throw new Exception("monitor server应答无效.");
            }
            totalLen -= 4;
            totalReceived = 0;
            buf = new byte[totalLen];
            while (totalReceived < totalLen) {
                int len = in.read(buf, totalReceived, totalLen - totalReceived);
                if (len <= 0) {
                    throw new Exception("monitor server应答无效.");
                }
                totalReceived += len;
            }
            logger.info("received monitor server response successfully.");

            Monitor.RespMonitor monitor = Monitor.RespMonitor.parseFrom(buf);
            if (monitor.getResult() != 0) {
                logger.error("monitor server returns errcode:"+monitor.getResult());
                throw new Exception("monitor server返回失败. result="+monitor.getResult());
            }

            Monitor.RespDelAlarm resp = monitor.getDelalarm();



        } finally {
            if (sock != null && sock.isConnected()) {
                try {
                    sock.close();
                } catch (Exception e) {
                }
            }
        }
        /*
        ArrayList<OneAlarm> alarmList = new ArrayList<OneAlarm>();
        long v = new Date().getTime();
        v = v - 3600;
        for (int i = 0; i < 9; ++i) {
            OneAlarm alarm = new OneAlarm();
            alarm.setOccur_date("20160101");
            alarm.setAttribute_name("attr#" + i);

            alarm.setLast_occur_time(Tools.TimeStamp2DateStr(v + i * 63, "HH:mm:ss") );
            alarm.setService_name(svc);
            alarm.setAlarm_type( (i%4)+1);
            alarmList.add(alarm);
        }


        return alarmList;
        */




    }
    

    public AlarmResponse exec(Alarm request)
    {
        AlarmResponse response = new AlarmResponse();

        Logger logger = Logger.getLogger(DeleteAlarm.class);

        String svc = "";
        String date = "";
        String attr = "";

        String result;




        svc = request.getService_name();
        date = request.getDate();
        attr = request.getAttr_name();

        if (svc == null || svc.length() == 0 ||
                date == null || date.length() == 0)
        {
            response.setMessage("svc name/date  should NOT be both empty!");
            response.setStatus(100);
            return response;
        }

        try {
            initMonitorIPAndPort();
             delAlarm(svc, attr, date, request.getLast_occur_time(), request.getAlarm_type());
            response.setMessage("success");
            response.setStatus(0);
            return response;

        }
        catch (Exception e)
        {
            response.setMessage(e.getMessage());
            response.setStatus(100);
            return response;
        }



    }
}
