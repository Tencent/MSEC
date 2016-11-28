
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

import beans.response.OneAttrChart;
import msec.monitor.Monitor;
import msec.org.Tools;
import org.apache.log4j.Logger;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.util.*;

/**
 * Created by Administrator on 2016/7/11.
 */
public class ServiceList {

    static  public void getServiceList(List<String> flsn, Map<String, List<String>> slsn) throws Exception
    {
        Socket sock = new Socket();
        Logger logger = Logger.getLogger(ServiceList.class);

        logger.info("begin to get attr value...");

        try {

            TreeMap<String, OneAttrChart> ret = new TreeMap<String, OneAttrChart>();
            sock.setSoTimeout(5000);
            sock.connect(new InetSocketAddress(MonitorBySvcOrIP.monitor_server_ip, MonitorBySvcOrIP.monitor_server_port), 3000);
            OutputStream out = sock.getOutputStream();

            Monitor.ReqTreeList.Builder b = Monitor.ReqTreeList.newBuilder();
            Monitor.ReqMonitor.Builder bb = Monitor.ReqMonitor.newBuilder();
            bb.setTreelist(b.build());
            Monitor.ReqMonitor req = bb.build();

            //发送长度信息
            byte[] lenField = Tools.int2Bytes(req.getSerializedSize() + 4);
            out.write(lenField);
            //发送实际的pb请求
            req.writeTo(out);
            //接收应答
            InputStream in = sock.getInputStream();
            byte[] buf = new byte[1024 * 1024];
            int totalLen = 4;
            int totalReceived = 0;
            while (totalReceived < totalLen) {
                int len = in.read(buf, totalReceived, totalLen - totalReceived);
                if (len <= 0) {
                    logger.error(String.format("read len field failed:%d", len));
                    throw new Exception("reading len field failed");
                }
                totalReceived += len;
            }

            totalLen = Tools.bytes2int(buf);

            logger.info("monitor server response size:" + totalLen);


            if (totalLen < 4 || totalLen > buf.length) {
                logger.error(String.format("totalLen invalid:%d", totalLen));
                throw new Exception("response totalLen is invalid");
            }
            totalLen -= 4;
            totalReceived = 0;
            buf = new byte[totalLen];
            while (totalReceived < totalLen) {
                int len = in.read(buf, totalReceived, totalLen - totalReceived);
                if (len <= 0) {
                    logger.error(String.format("read response body failed:%d", len));
                    throw new Exception("reading response body failed!");
                }
                totalReceived += len;
            }

            Monitor.RespMonitor monitor = Monitor.RespMonitor.parseFrom(buf);
            if (monitor.getResult() != 0) {
                logger.error("monitor returns error code:" + monitor.getResult());
                throw new Exception("monitor returns error code " + monitor.getResult());
            }
            Monitor.RespTreeList treeList = monitor.getTreelist();
            for (int i = 0; i < treeList.getInfosCount(); i++) {
                Monitor.TreeListInfo info = treeList.getInfos(i);
                String svcname = info.getServicename();
                int pos = svcname.indexOf(".");
                String first_level = ".unnamed";
                String second_level = svcname;
                if (pos > 0 && pos < (svcname.length() - 1)) {
                    first_level = svcname.substring(0, pos);
                    second_level = svcname.substring(pos + 1);
                }

                if (slsn.get(first_level) == null) {
                    slsn.put(first_level, new ArrayList<String>());
                    slsn.get(first_level).add(second_level);

                    flsn.add(first_level);
                }
                else
                {
                    slsn.get(first_level).add(second_level);
                }

            }
            Collections.sort(flsn);
            for (int i = 0; i < flsn.size() ; i++) {
                List<String> second_level = slsn.get(flsn.get(i));
                Collections.sort(second_level);
            }
        }
        finally {
            sock.close();
        }
    }
}
