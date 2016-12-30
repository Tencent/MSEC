
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


package api.lb.msec.org;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Created by Administrator on 2016/5/18.
 */

public class AccessLB {
    private native boolean getroutebyname(String name, byte[] ip, byte[] port, byte[] type);
    public native boolean updateroute(String name, String ip, int failed, int cost);

    public boolean updateroutebyname(String name, String ip, int failed, int cost)
    {
        return updateroute(name, ip, failed, cost);
    }
    
    public boolean getroutebyname(String name,Route r ) throws Exception
    {
        if (System.getProperties().getProperty("os.name").toUpperCase().indexOf("WINDOWS") >= 0) {
            r.setIp("127.0.0.1");
            r.setPort(7963);
            r.setComm_type(Route.COMM_TYPE.COMM_TYPE_TCP);
            return true;
        }
        // c代码返回的数据用byte array的方式传递
        byte[] ip = new byte[100];
        byte[] port = new byte[100];
        byte[] type = new byte[100];
        int ascii_str_len = 0;
        //memset 0一下
        for (int i = 0; i < 100; ++i)
        {
            ip[i] = 0;
            port[i] = 0;
            type[i] = 0;
        }
        //调用 c代码
        boolean result = getroutebyname(name, ip, port, type);
        if (!result)//失败
        {
            System.out.println("call native method failed.");
            return false;
        }

        //获得IP字符数组的长度
        ascii_str_len = 0;
        while (ip[ascii_str_len] != 0 && ascii_str_len < 100)
        {
            ascii_str_len++;
        }
        r.setIp(new String(ip, 0, ascii_str_len, "ascii"));

        //获得port字符数组的长度
        ascii_str_len = 0;
        while (port[ascii_str_len] != 0 && ascii_str_len < 100)
        {
            ascii_str_len++;
        }
        int iPort = new Integer(new String(port, 0, ascii_str_len, "ascii")).intValue();
        r.setPort(iPort);

        //获得type字符数组的长度
        ascii_str_len = 0;
        while (type[ascii_str_len] != 0 && ascii_str_len < 100)
        {
            ascii_str_len++;
        }
        String sType = new String(type, 0, ascii_str_len, "ascii");
        if (sType.equals("udp"))
        {
            r.setComm_type(Route.COMM_TYPE.COMM_TYPE_UDP);
        }
        if (sType.equals("tcp"))
        {
            r.setComm_type(Route.COMM_TYPE.COMM_TYPE_TCP);
        }
        if (sType.equals("all"))
        {
            r.setComm_type(Route.COMM_TYPE.COMM_TYPE_ALL);
        }

        return true;
    }
    static
    {
        if (System.getProperties().getProperty("os.name").toUpperCase().indexOf("WINDOWS") < 0) {
            try {
                InputStream in = AccessLB.class.getClass().getResourceAsStream("/sofiles/libjni_lb.so");
                File f = File.createTempFile("libjni_lb", ".so");
                OutputStream out = new FileOutputStream(f);
                byte[] buf = new byte[10240];
                while (true) {
                    int len = in.read(buf);
                    if (len <= 0) {
                        break;
                    }
                    out.write(buf, 0, len);
                }
                in.close();
                out.close();
                System.load(f.getAbsolutePath());
                f.delete();
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
	
	public static void main(String[] args)
	{
		System.out.println("main begin.");
		AccessLB ac = new AccessLB();
		Route r = new Route();
		try {
		     boolean ret = ac.getroutebyname("RESERVED.monitor", r);
		System.out.println(ret + " addr: " + r.getIp() + ":" + r.getPort());
		} catch (Exception ex) {
			System.out.println(ex);
		}
	}

}
