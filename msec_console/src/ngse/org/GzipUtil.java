
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


package ngse.org;

import org.apache.commons.compress.compressors.gzip.GzipCompressorInputStream;
import org.apache.commons.compress.compressors.gzip.GzipCompressorOutputStream;

import java.io.FileInputStream;
import java.io.FileOutputStream;

/**
 * Created by Administrator on 2016/4/7.
 */
public class GzipUtil {
    static public void zip(String srcFile) throws  Exception
    {
        GzipCompressorOutputStream out = new GzipCompressorOutputStream(new FileOutputStream(srcFile+".gz"));
        FileInputStream in = new FileInputStream(srcFile);
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
        out.flush();
        out.close();
        in.close();
    }
    static public void zip(String srcFile, String destFile) throws Exception
    {
        GzipCompressorOutputStream out = new GzipCompressorOutputStream(new FileOutputStream(destFile));
        FileInputStream in = new FileInputStream(srcFile);
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
        out.flush();
        out.close();
        in.close();
    }
    static public void unzip(String srcFile) throws Exception
    {
        GzipCompressorInputStream in = new GzipCompressorInputStream(new FileInputStream(srcFile));
        int index = srcFile.indexOf(".gz");
        String destFile = "";
        if (index == srcFile.length()-3)
        {
            destFile = srcFile.substring(0, index);
        }
        else
        {
            destFile = srcFile+".decompress";
        }
        FileOutputStream out = new FileOutputStream(destFile);
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
        out.flush();
        out.close();
        in.close();
    }
}
