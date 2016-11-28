
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


package api.monitor.ngse.org;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Created by Administrator on 2016/5/19.
 */
public class AccessMonitor {
    static
    {
        //System.loadLibrary("jni_monitor");
        try {
            InputStream in = AccessMonitor.class.getClass().getResourceAsStream("/sofiles/libjni_monitor.so");
            File f = File.createTempFile("libjni_monitor", ".so");
            OutputStream out = new FileOutputStream(f);
            byte[] buf = new byte[10240];
            while (true)
            {
                int len = in.read(buf);
                if (len <= 0)
                {
                    break;
                }
                out.write(buf, 0, len);
            }
            in.close();;
            out.close();
            System.load(f.getAbsolutePath());
            f.delete();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
    public native boolean init(String fileName);
    public native boolean add(String serviceName, String attrName, int value);
    public native boolean set(String serviceName, String attrName, int value);
}
