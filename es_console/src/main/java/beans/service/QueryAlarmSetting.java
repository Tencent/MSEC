
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

import beans.request.Alarm;
import beans.response.AlarmResponse;
import beans.response.OneAlarm;
import msec.monitor.Monitor;
import msec.org.DBUtil;
import msec.org.JsonRPCHandler;
import msec.org.Tools;
import org.apache.log4j.Logger;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.ArrayList;

/**
 *
 */
public class QueryAlarmSetting extends JsonRPCHandler {


    static public ArrayList<OneAlarm>  getAlarmSettingList(String svc) throws Exception
    {

        Logger logger = Logger.getLogger(QueryAlarmSetting.class);


        Socket sock = new Socket();
        ArrayList<String> ret = new ArrayList<String>();
        try {
            sock.setSoTimeout(2000);
            sock.connect(new InetSocketAddress(MonitorBySvcOrIP.monitor_server_ip, MonitorBySvcOrIP.monitor_server_port), 3000);

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
                throw new Exception("monitor server应答失败.");
            }

            Monitor.RespService resp = monitor.getService();
            ArrayList<OneAlarm> alarmList = new ArrayList<OneAlarm>();
            logger.info("monitor returns alarm number:"+resp.getAlarmattrsCount());
            for (int i = 0; i < resp.getAlarmattrsCount(); ++i) {

                if (resp.getAlarmattrs(i).getMax() > 0) {
                    OneAlarm alarm = new OneAlarm();

                    alarm.setAttribute_name(resp.getAlarmattrs(i).getAttrname());
                    alarm.setService_name(svc);
                    alarm.setAlarm_type(Monitor.AlarmType.ALARM_MAX.getNumber());
                    alarm.setThreshold(resp.getAlarmattrs(i).getMax());

                    alarmList.add(alarm);
                }
                if (resp.getAlarmattrs(i).getMin() > 0) {
                    OneAlarm alarm = new OneAlarm();

                    alarm.setAttribute_name(resp.getAlarmattrs(i).getAttrname());
                    alarm.setService_name(svc);
                    alarm.setAlarm_type(Monitor.AlarmType.ALARM_MIN.getNumber());
                    alarm.setThreshold(resp.getAlarmattrs(i).getMin());

                    alarmList.add(alarm);
                }
                if (resp.getAlarmattrs(i).getDiff() > 0) {
                    OneAlarm alarm = new OneAlarm();

                    alarm.setAttribute_name(resp.getAlarmattrs(i).getAttrname());
                    alarm.setService_name(svc);
                    alarm.setAlarm_type(Monitor.AlarmType.ALARM_DIFF.getNumber());
                    alarm.setThreshold(resp.getAlarmattrs(i).getDiff());

                    alarmList.add(alarm);
                }
                if (resp.getAlarmattrs(i).getDiffPercent() > 0) {
                    OneAlarm alarm = new OneAlarm();

                    alarm.setAttribute_name(resp.getAlarmattrs(i).getAttrname());
                    alarm.setService_name(svc);
                    alarm.setAlarm_type(Monitor.AlarmType.ALARM_DIFF_PERCENT.getNumber());
                    alarm.setThreshold(resp.getAlarmattrs(i).getDiffPercent());

                    alarmList.add(alarm);
                }

            }

            return alarmList;

        } finally {
            if (sock != null && sock.isConnected()) {
                try {
                    sock.close();
                } catch (Exception e) {
                }
            }
        }





    }


    public AlarmResponse exec(Alarm request)
    {
        AlarmResponse response = new AlarmResponse();

        Logger logger = Logger.getLogger(QueryAlarmSetting.class);

        String svc = "";
        String date = "";

        String result;
        result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }

        svc = request.getService_name();
        date = request.getDate();

        if (    svc == null || svc.length() == 0)
        {
            response.setMessage("service name  should NOT be empty!");
            response.setStatus(100);
            return response;
        }

        try {

            ArrayList<OneAlarm> alarmList = getAlarmSettingList(svc);
            response.setAlarm_list(alarmList);
        }
        catch (Exception e)
        {
            response.setMessage(e.getMessage());
            response.setStatus(100);
            return response;
        }


        response.setMessage("success");
        response.setStatus(0);
        return response;
    }
}
