
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
import beans.dbaccess.LibraryFile;
import beans.dbaccess.SecondLevelServiceConfigTag;
import beans.dbaccess.SharedobjectTag;
import beans.request.DevPackage;
import beans.request.ReleasePlan;
import ngse.org.*;
import org.apache.log4j.Logger;

import javax.servlet.ServletContext;
import java.io.*;
import java.util.ArrayList;
import java.util.List;


/**
 * Created by Administrator on 2016/2/16.
 * 打开发包，得到一个tar文件
 */
public class PackDevFile implements Runnable {
    DevPackage pack;

    ServletContext servletContext;

    String outputFileName ;
    String resultString = "";

    public void setOutputFileName(String outputFileName) {
        this.outputFileName = outputFileName;
    }

    public String getResultString() {
        return resultString;
    }

    public void setResultString(String resultString) {
        this.resultString = resultString;
    }

    public String getOutputFileName()
    {
        return outputFileName;
    }

    public PackDevFile(DevPackage p, ServletContext context)
    {
        pack = p;
        servletContext = context;
    }
    private ArrayList<LibraryFile> getLibraryFilesFromDB(String flsn, String slsn)
    {
        Logger logger = Logger.getLogger(PackDevFile.class);

        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            return null;
        }

        try {

            String sql = "select file_name from t_library_file where first_level_service_name=? and second_level_service_name=?";
            List<Object> params = new ArrayList<Object>();
            params.add(flsn);
            params.add(slsn);



            ArrayList<LibraryFile> ret = util.findMoreRefResult(sql, params, LibraryFile.class);
            logger.info("get library files from db, file number:"+ret.size());
            return ret;
        }
        catch (Exception e)
        {
            e.printStackTrace();
            logger.error(e.toString());
            return null;
        }
        finally {
            util.releaseConn();
        }
    }

    private void mkdirs(String baseDir) throws Exception
    {
        File f = new File(baseDir);
        if (!f.exists()) {f.mkdirs();}

        String s = null;

        s = baseDir+File.separator+"bin";
        f = new File(s);
        if (!f.exists()) {f.mkdirs();}

        s = baseDir+File.separator+"cnf";
        f = new File(s);
        if (!f.exists()) {f.mkdirs();}

        s = baseDir+File.separator+"lib";
        f = new File(s);
        if (!f.exists()) {f.mkdirs();}

        s = baseDir+File.separator+"so";
        f = new File(s);
        if (!f.exists()) {f.mkdirs();}


    }
    private boolean copyFile(File frm, File to)
    {

        try {
            FileInputStream in = new FileInputStream(frm);
            FileOutputStream out = new FileOutputStream(to);
            byte[] buf = new byte[1024*8];
            while (true)
            {
                int len = in.read(buf);
                if (len <= 0)
                {
                    break;
                }
                out.write(buf, 0, len);
            }
            in.close();
            out.close();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }

    }

    private void copyCnfFile(String baseDir) throws Exception
    {
        Logger logger = Logger.getLogger(PackDevFile.class);

        String destFile = baseDir+File.separator+"etc"+File.separator+"service.yaml";
        String srcFile = SecondLevelServiceConfigTag.getConfigFileName(pack.getFirst_level_service_name(),
                pack.getSecond_level_service_name(), pack.getConfig_tag());
        if (copyFile(new File(srcFile), new File(destFile)))
        {
            logger.info("copy file successfully."+srcFile+" "+destFile);
        }
        else
        {
            logger.error("failed to copy file:"+srcFile+" "+destFile);
            throw  new Exception("failed to copy file:"+srcFile+" "+destFile);
        }

    }
    private void copyLibraryFile(String baseDir) throws Exception
    {
        Logger logger = Logger.getLogger(PackDevFile.class);

        ArrayList<LibraryFile> libraryFiles = getLibraryFilesFromDB(pack.getFirst_level_service_name(), pack.getSecond_level_service_name());


        for (int i = 0; i < libraryFiles.size(); i++) {
            String destFile = baseDir+"/lib/"+libraryFiles.get(i).getFile_name();
            String srcFile = LibraryFile.getLibraryFileName(pack.getFirst_level_service_name(),
                    pack.getSecond_level_service_name(), libraryFiles.get(i).getFile_name());
            if (copyFile(new File(srcFile), new File(destFile)))
            {
                logger.info("copy file successfully."+srcFile+" "+destFile);
                continue;
            }
            else
            {
                logger.error("failed to copy file:"+srcFile+" "+destFile);
                throw  new Exception("failed to copy file:"+srcFile+" "+destFile);
            }
        }

    }
    private void copySharedobject(String baseDir) throws Exception
    {
        Logger logger = Logger.getLogger(PackDevFile.class);

        String suffix = "so";
        String destFile = baseDir+"/bin/msec.so";
        String srcFile = SharedobjectTag.getSharedobjectName(pack.getFirst_level_service_name(),
                pack.getSecond_level_service_name(), pack.getSharedobject_tag(), suffix);
        if (copyFile(new File(srcFile), new File(destFile)))
        {
            logger.info("copy file successfully."+srcFile+" "+destFile);

        }
        else
        {
            logger.error("failed to copy file:"+srcFile+" "+destFile);
           throw new Exception("failed to copy file:"+srcFile+" "+destFile);
        }
    }
    private void copyIDLFile(String baseDir) throws Exception
    {

        Logger logger = Logger.getLogger(PackDevFile.class);
      //  String destFile = baseDir+"/"+pack.getIdl_tag()+".proto";
        String destFile = baseDir+"/msec.proto";

        String srcFile = IDL.getIDLFileName(pack.getFirst_level_service_name(),
                pack.getSecond_level_service_name(),
                pack.getIdl_tag());
        if (copyFile(new File(srcFile), new File(destFile)))
        {
            logger.info("copy file successfully."+srcFile+" "+destFile);

        }
        else
        {
            logger.error("failed to copy file:"+srcFile+" "+destFile);
            throw  new Exception("failed to copy file:"+srcFile+" "+destFile);
        }
    }



    private void mktar(String baseDir, String tarFileName) throws Exception
    {
        Logger logger = Logger.getLogger(PackDevFile.class);

        TarUtil.archive(baseDir, tarFileName);

        logger.info("tar successfully." + tarFileName);

    }

    private void getSppFromResource(String rnd, String baseDir) throws Exception
    {
        Logger logger = Logger.getLogger(PackDevFile.class);
        //将war包资源中的spp.tar解压到目录baseDir
        InputStream inputStream = servletContext.getResourceAsStream("/resource/spp_dev.tar");
        String tmpTarFile = ServletConfig.fileServerRootDir + "/tmp/spp_dev_"+rnd+".tar";

        logger.info("copy spp from resource to "+tmpTarFile);

        File f = null;

        //将资源文件拷贝到文件系统中的普通文件，并解压缩为目录baseDir

        OutputStream outputStream = new FileOutputStream(tmpTarFile);
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


        //把普通文件解包到目录baseDir
        TarUtil.dearchive(new File(tmpTarFile), new File(baseDir));
        logger.info("dearchive tar to " + baseDir);

        //删除普通文件
        new File(tmpTarFile).delete();


    }
    private void getJavaFrameworkFromResource(String rnd, String baseDir) throws Exception
    {
        Logger logger = Logger.getLogger(PackDevFile.class);

        InputStream inputStream = servletContext.getResourceAsStream("/resource/java_dev.tar");
        String tmpTarFile = ServletConfig.fileServerRootDir + "/tmp/java_dev_"+rnd+".tar";

        logger.info("copy java framework from resource to "+tmpTarFile);

        File f = null;

        //将资源文件拷贝到文件系统中的普通文件，并解压缩为目录baseDir

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

        //删除普通文件
        new File(tmpTarFile).delete();



    }



    @Override
    public void run()  {

        Logger logger = Logger.getLogger(PackDevFile.class);

        String rnd = Tools.nowString("yyyyMMddHHmmss");
        String baseDir = ServletConfig.fileServerRootDir+ File.separator+"tmp"+File.separator+"DevPackage_"+pack.getSecond_level_service_name()+rnd;
        String tarFileName = baseDir +".tar";
        String gzFileName = tarFileName +".gz";
        String result;

        try {
            if (pack.getDev_lang().equals("c++")) {
                getSppFromResource(rnd, baseDir);
            } else if (pack.getDev_lang().equals("java")) {
                getJavaFrameworkFromResource(rnd, baseDir);
            } else if (pack.getDev_lang().equals("php")) {
                getSppFromResource(rnd, baseDir);
            } else if (pack.getDev_lang().equals("python")) {
                getSppFromResource(rnd, baseDir);
            } else {
                throw  new Exception("invalid dev lang " + pack.getDev_lang());
            }

            copyLibraryFile(baseDir);

            copyIDLFile(baseDir);



            //执行python脚本，生成目录
            String[] cmd = new String[4];
            String pythonScript;
            String outputDir;
            if (pack.getDev_lang().equals("c++")) {
                pythonScript = baseDir + "/rpc/template/create_rpc.py";
                new File(pythonScript).setExecutable(true);

                cmd[0] = pythonScript;
                cmd[1] = baseDir + "/msec.proto";
                cmd[2] = baseDir;
                outputDir =  baseDir + "/"+pack.getFirst_level_service_name()+"."+pack.getSecond_level_service_name();
                cmd[3] = outputDir;
            }
            else if (pack.getDev_lang().equals("java"))
            {
                pythonScript = baseDir + "/create_rpc.py";
                new File(pythonScript).setExecutable(true);

                cmd[0] = pythonScript;
                cmd[1] =  "msec.proto";
                cmd[2] = baseDir;
                outputDir =  baseDir;
                cmd[3] = pack.getFirst_level_service_name()+"."+pack.getSecond_level_service_name();;
            }
            else if (pack.getDev_lang().equals("php"))
            {
                pythonScript = baseDir + "/rpc/php_template/create_rpc.py";
                new File(pythonScript).setExecutable(true);

                cmd[0] = pythonScript;
                cmd[1] = baseDir + "/msec.proto";
                cmd[2] = baseDir;
                outputDir =  baseDir + "/"+pack.getFirst_level_service_name()+"."+pack.getSecond_level_service_name();
                cmd[3] = outputDir;
            }
            else if (pack.getDev_lang().equals("python"))
            {
                pythonScript = baseDir + "/rpc/py_template/create_rpc.py";
                new File(pythonScript).setExecutable(true);

                cmd[0] = pythonScript;
                cmd[1] = baseDir + "/msec.proto";
                cmd[2] = baseDir;
                outputDir =  baseDir + "/"+pack.getFirst_level_service_name()+"."+pack.getSecond_level_service_name();
                cmd[3] = outputDir;
            }
            else
            {
                throw new Exception("invalid dev lang"+pack.getDev_lang());
            }

            StringBuffer cmdResult = new StringBuffer();
            if (Tools.runCommand(cmd, cmdResult, true) != 0)
            {
                logger.error(cmdResult.toString());
                throw  new Exception("create_rpc.py failed:"+cmdResult.toString());

            }
            logger.info("run python script successfully.");




            mktar(outputDir, tarFileName);

            GzipUtil.zip(tarFileName, gzFileName);
            logger.info("make tar file successfully.");
            outputFileName = gzFileName;
            setResultString("success");





        }
        catch (Exception e)
        {
            e.printStackTrace();
            logger.error(e.getMessage());
            setResultString(e.getMessage());
        }
        finally {
            Tools.deleteDirectory(new File(baseDir));
        }

    }
}
