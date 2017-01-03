
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

        InputStream inputStream = this.getServlet().getServletContext().getResourceAsStream("/resource/3rd_API.tar.gz");
        String destFile = destDir + File.separator + "3rd_API.tar.gz";

        logger.info("copy 3rd API to " + destFile);

        File f = null;
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
    private  void genPhpCode(String protofile, String outputDir) throws Exception
    {
        Logger logger = Logger.getLogger(PackDevFile.class);


        String rnd = ""+(int)(Math.random() * 1000000);
        InputStream inputStream = getServlet().getServletContext().getResourceAsStream("/resource/spp_dev.tar");
        String tmpTarFile = ServletConfig.fileServerRootDir + "/tmp/genPhpCode"+rnd+".tar";
        String baseDir = ServletConfig.fileServerRootDir + "/tmp/genPhpCode"+rnd+"_basedir";


        logger.info("copy php framework from resource to "+tmpTarFile);

        File f = null;

        //将资源文件拷贝到文件系统中的普通文件，并解压缩为目录baseDir
        try
        {
            byte[] buf = new byte[10240];
            int len;
            OutputStream outputStream = new FileOutputStream(tmpTarFile);
            while (true) {


                len = inputStream.read(buf);

                if (len <= 0) {
                    break;
                }
                outputStream.write(buf, 0, len);

            }
            outputStream.close();
            inputStream.close();


            //把普通文件解包到目录baseDir
            TarUtil.dearchive(new File(tmpTarFile), new File(baseDir));
            logger.info("dearchive tar to " + baseDir);

            //执行python脚本产生php代码
            String[] cmd = new String[5];
            String pythonScript;
            pythonScript = baseDir + "/rpc/php_template/create_rpc.py";
            new File(pythonScript).setExecutable(true);

            cmd[0] = pythonScript;
            cmd[1] = protofile;
            cmd[2] = baseDir;
            cmd[3] = outputDir;
            cmd[4] = "--protoc";

            StringBuffer cmdResult = new StringBuffer();
            if (Tools.runCommand(cmd, cmdResult, true) != 0)
            {
                throw  new Exception("gene php code failed:"+cmdResult);

            }
            logger.info("run python script successfully.");

        }
        finally {
            //删除普通文件
            try {
                new File(tmpTarFile).delete();
                Tools.deleteDirectory(new File(baseDir));
            }
            catch (Exception e){}
        }

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
        String cpp_out = destDir + File.separator+"cpp";
        String java_out = destDir + File.separator+"java";
        String php_out = destDir + File.separator+"php";
        String py_out = destDir + File.separator+"python";

        logger.info("code generation destDir="+destDir);

        File f = new File(destDir);
        if (f.isFile())
        {
            f.delete();
        }
        else if  (f.isDirectory())
        {
            Tools.deleteDirectory(f);
        }

        f.mkdir();
        new File(cpp_out).mkdir();
        new File(java_out).mkdir();
        new File(php_out).mkdir();
        new File(py_out).mkdir();
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
            String newIDLFileName = destDir + File.separator+ flsn+"_"+slsn+".proto";
            Tools.copyFile(idlFileName, newIDLFileName);
            String[] cmd = {"protoc", "--cpp_out=" + cpp_out, "--java_out=" + java_out,"--python_out="+py_out, "-I"+destDir, newIDLFileName};
            StringBuffer sb = new StringBuffer();
            int v = Tools.runCommand(cmd, sb, true);
            if (v != 0)
            {
                throw new Exception("Cmd protoc failed!"+sb.toString());
            }
            logger.info(String.format("protoc compile:%d, %s", v, sb.toString()));
            // pb这个版本不支持直接生成php的代码，只好自己实现
            genPhpCode(newIDLFileName,php_out);

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
