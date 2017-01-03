
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
public class AddAlarmSetting extends JsonRPCHandler {
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
            String sql =sql = "select ip,port from t_second_level_service_ipinfo where second_level_service_name='monitor' and " +
                    "first_level_service_name='RESERVED' and status='enabled'";


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

    static public void  addAlarmSetting(String svc, String attr,  int type, int threshold ) throws Exception
    {

        Logger logger = Logger.getLogger(AddAlarmSetting.class);


        Socket sock = new Socket();
        ArrayList<String> ret = new ArrayList<String>();
        try {
            sock.setSoTimeout(2000);
            sock.connect(new InetSocketAddress(monitor_server_ip, monitor_server_port), 3000);

            OutputStream out = sock.getOutputStream();

            //发送请求
            //获取指定svc下的所有ip
            //组包
            Monitor.ReqSetAlarmAttr.Builder b = Monitor.ReqSetAlarmAttr.newBuilder();
            b.setServicename(svc);
            b.setAttrname(attr);
            switch (type)
            {
                case Monitor.AlarmType.ALARM_DIFF_PERCENT_VALUE:
                    b.setDiffPercent(threshold);
                    break;
                case Monitor.AlarmType.ALARM_MAX_VALUE:
                    b.setMax(threshold);
                    break;
                case Monitor.AlarmType.ALARM_DIFF_VALUE:
                    b.setDiff(threshold);
                    break;
                case Monitor.AlarmType.ALARM_MIN_VALUE:
                    b.setMin(threshold);
                    break;
                default:
                    throw new Exception("invalid type:"+type);
            }
            Monitor.ReqSetAlarmAttr reqSetAlarmAttr = b.build();

            Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
            bb.setSetalarmattr(reqSetAlarmAttr);
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

            Monitor.RespSetAlarmAttr resp = monitor.getSetalarmattr();




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

        Logger logger = Logger.getLogger(AddAlarmSetting.class);

        String svc = "";
        String attr = "";






        svc = request.getService_name();
        attr = request.getAttr_name();
        int type = request.getAlarm_type();
        int threshold = request.getThreshold();

        if (svc == null || svc.length() == 0 )
        {
            response.setMessage("svc name  should NOT be both empty!");
            response.setStatus(100);
            return response;
        }

        try {
            initMonitorIPAndPort();
             addAlarmSetting(svc, attr, type, threshold);
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
