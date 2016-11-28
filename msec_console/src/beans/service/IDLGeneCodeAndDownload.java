
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

import beans.dbaccess.IDL;

import ngse.org.*;
import org.apache.log4j.Logger;

import javax.servlet.ServletOutputStream;
import java.io.*;


/**
 * Created by Administrator on 2016/2/17.
 * 根据某个IDL版本，生成代码并下载下去
 */
public class IDLGeneCodeAndDownload extends JsonRPCHandler {


    private void copy3rdAPI(String destDir) throws Exception
    {
        Logger logger = Logger.getLogger(IDLGeneCodeAndDownload.class);
        //将war包资源中的spp.tar解压到目录baseDir
        InputStream inputStream = this.getServlet().getServletContext().getResourceAsStream("/resource/3rd_API.zip");
        String destFile = destDir + File.separator + "3rd_API.zip";

        logger.info("copy 3rd API to " + destFile);

        File f = null;

        //将资源文件拷贝到文件系统中的普通文件，并解压缩为目录baseDir
        OutputStream outputStream = new FileOutputStream(destFile);
        byte[] buf = new byte[10240];
        int len;
        while (true) {
            len = inputStream.read(buf);

            if (len <= 0) {
                break;
            }
            outputStream.write(buf, 0, len);

        }
        outputStream.close();
        inputStream.close();



    }

    public JsonRPCResponseBase exec(IDL request)
    {
        JsonRPCResponseBase response = new JsonRPCResponseBase();
        Logger logger = Logger.getLogger(this.getClass());

        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }

        String flsn = request.getFirst_level_service_name();
        String slsn = request.getSecond_level_service_name();
        String tag = request.getTag_name();
        if (flsn == null || flsn.length() < 1 ||
                slsn == null || slsn.length() < 1||
                tag == null || tag.length() < 1)
        {
            response.setMessage("service name/tag is empty!");
            response.setStatus(100);
            return response;
        }
        // 调用外部的protoc命令，生成代码保存到指定目录
        logger.info("call protoc to generate code");
        String rnd = String.format("%d", (int)( Math.random()*Integer.MAX_VALUE) );
        String destDir = ServletConfig.fileServerRootDir+ File.separator+"tmp"+File.separator+"IDLGeneCode_"+slsn+"_"+rnd;

        logger.info("code generation destDir="+destDir);

        File f = new File(destDir);
        if (!f.exists())
        {
            f.mkdirs();
        }
        //将第三方（例如cgi）要用到的库拷贝到目标目录
        try
        {
            copy3rdAPI(destDir);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            response.setMessage("copy 3rd API failed"+e.getMessage());
            response.setStatus(100);
            return response;
        }

        String tarFileName = destDir +".tar";
        try {
            // protoc 命令生成代码
            String idlFileName = IDL.getIDLFileName(flsn, slsn, tag);
            String newIDLFileName = destDir + "/msec.proto";
            Tools.copyFile(idlFileName, newIDLFileName);
            String[] cmd = {"protoc", "--cpp_out=" + destDir, "--java_out=" + destDir, "-I"+destDir, newIDLFileName};
            StringBuffer sb = new StringBuffer();
            int v = Tools.runCommand(cmd, sb, true);
            if (v != 0)
            {
                new Exception("Cmd protoc failed!");
            }
            logger.info(String.format("protoc compile:%d, %s", v, sb.toString()));

            //打成tar包
            logger.info("generate tar file...");

            TarUtil.archive(destDir, tarFileName);
            Tools.deleteDirectory(new File(destDir));
        }
        catch (Exception e)
        {
            e.printStackTrace();
            response.setMessage(e.getMessage());
            response.setStatus(100);
            return response;
        }

        //把tar包下载下去

        f = new File(tarFileName);
        String length = String.format("%d", f.length());

        logger.info("download tar file, file length:"+f.length());

        getHttpResponse().setHeader("Content-disposition", "attachment;filename=" + f.getName());
        // set the MIME type.
        getHttpResponse().setContentType("application/x-tar");
        getHttpResponse().setHeader("Content_Length", length);

        try {
            ServletOutputStream out = getHttpResponse().getOutputStream();
            FileInputStream fileInputStream = new FileInputStream(f);
            byte[] buf = new byte[10240];
            while (true)
            {
                int len = fileInputStream.read(buf);
                if (len <= 0)
                {
                    break;
                }
                out.write(buf,0, len);
            }

            out.close();

            fileInputStream.close();

            f.delete();
            return null;
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        return response;
    }
}
