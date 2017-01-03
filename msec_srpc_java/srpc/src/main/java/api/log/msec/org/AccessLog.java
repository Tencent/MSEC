
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

import org.msec.rpc.RequestProcessor;
import org.msec.rpc.RpcContext;

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
        if (System.getProperties().getProperty("os.name").toUpperCase().indexOf("WINDOWS") < 0) {
            try {
                InputStream in = AccessLog.class.getClass().getResourceAsStream("/sofiles/libjni_log.so");
                File f = File.createTempFile("libjni_log", ".so");
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

    private AccessLog() {}

    private static AccessLog s_instance = new AccessLog();
    public static AccessLog getInstance() { return s_instance; }

    public static synchronized boolean initLog(String configFile)
    {
        return s_instance.init(configFile);
    }

    public static synchronized void log(int level, Map<String, String> headers, String body)
    {
        if (headers != null) {
            for (Map.Entry<String, String> entry : headers.entrySet())
            {
                s_instance.setHeader(entry.getKey(), entry.getValue());
            }
        }
        s_instance.log(level, body);
    }

    public static void doLog(int level, String body) {
        StackTraceElement  ste = Thread.currentThread().getStackTrace()[2];
        RpcContext  context = (RpcContext)RequestProcessor.getThreadContext("session");
        if (context == null) {
            s_instance.log(level, null, body);
        } else {
            context.getLogOptions().put("FileLine", ste.getFileName() + ":" + ste.getLineNumber());
            context.getLogOptions().put("Function", ste.getClassName() + ":" + ste.getMethodName());
            s_instance.log(level, context.getLogOptions(), body);
        }
    }

    public static void doLog(int level, Map<String, String> headers, String body) {
        StackTraceElement  ste = Thread.currentThread().getStackTrace()[2];
        RpcContext  context = (RpcContext)RequestProcessor.getThreadContext("session");
        if (context != null) {
            for (Map.Entry<String, String> entry : context.getLogOptions().entrySet()) {
                headers.put(entry.getKey(), entry.getValue());
            }
        }

        headers.put("FileLine", ste.getFileName() + ":" + ste.getLineNumber());
        headers.put("Function", ste.getClassName() + ":" + ste.getMethodName());
        s_instance.log(level, headers, body);
    }
}
