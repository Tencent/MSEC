
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


package api.log.msec.org;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Map;

/**
 * Created by Administrator on 2016/5/19.
 */
public class AccessLog {
    static public final int LOG_LEVEL_TRACE = 0;
    static public final int LOG_LEVEL_DEBUG = 1;
    static public final int LOG_LEVEL_INFO = 2;
    static public final int LOG_LEVEL_ERROR = 3;
    static public final int LOG_LEVEL_FATAL = 4;


    public native boolean init(String configFile);
    public native void setHeader(String key, String value);
    public native void log(int level, String body);

    static
    {
        try {
            InputStream in = AccessLog.class.getClass().getResourceAsStream("/sofiles/libjni_log.so");
            File f = File.createTempFile("libjni_log", ".so");
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
        //System.loadLibrary("jni_log");
    }

    public void log(int level, Map<String, String> headers, String body)
    {
        for (Map.Entry<String, String> entry : headers.entrySet())
        {
            setHeader(entry.getKey(), entry.getValue());
        }
        log(level, body);
    }
}
