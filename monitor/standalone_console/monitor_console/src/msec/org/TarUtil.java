
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


package msec.org;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.file.*;

import org.apache.commons.compress.archivers.tar.TarArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveOutputStream;
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream;
import org.apache.commons.compress.utils.IOUtils;

//打包和拆包.tar文件的工具类
public abstract class TarUtil {




    private static final int BUFFERSZ = 1024;

    //将文件或者目录srcPath打包成一个tar文件，保存在destPath文件
    // 用到了递归的方式遍历目录下的文件和目录
    public static void archive(String srcPath, String destPath)
            throws Exception {

        File srcFile = new File(srcPath);
        File destFile = new File(destPath);

        archive(srcFile, destFile);

    }

    public static void archive(File srcFile, File destFile) throws Exception {

        TarArchiveOutputStream taos = new TarArchiveOutputStream(
                new FileOutputStream(destFile));
        taos.setLongFileMode(TarArchiveOutputStream.LONGFILE_GNU);

        archive(srcFile, taos, "");

        taos.flush();
        taos.close();
    }

    private static void archive(File srcFile, TarArchiveOutputStream taos,
                                String basePath) throws Exception {
        if (srcFile.isDirectory()) {
            archiveDir(srcFile, taos, basePath);
        } else {
            archiveFile(srcFile, taos, basePath);
        }
    }

    private static void archiveDir(File dir, TarArchiveOutputStream taos,
                                   String basePath) throws Exception {

        File[] files = dir.listFiles();

        if (files.length < 1) {
            TarArchiveEntry entry = new TarArchiveEntry(basePath
                    + dir.getName() + File.separator);

            taos.putArchiveEntry(entry);
            taos.closeArchiveEntry();
        }

        for (File file : files) {

            // 递归归档
            archive(file, taos, basePath + dir.getName() + File.separator);

        }
    }


    private static void archiveFile(File file, TarArchiveOutputStream taos,
                                    String dir) throws Exception {


        boolean is_symbolic = Files.isSymbolicLink(file.toPath());
        TarArchiveEntry entry;
        if(is_symbolic) {
            entry = new TarArchiveEntry(dir + file.getName(), TarArchiveEntry.LF_SYMLINK);
            entry.setLinkName(Files.readSymbolicLink(file.toPath()).toString());
        }
        else {
            entry = new TarArchiveEntry(dir + file.getName());
            entry.setSize(file.length());
            if (file.canExecute()) {
                // -rwxr-xr-x
                entry.setMode(493);
            }
        }
        taos.putArchiveEntry(entry);

        if(!is_symbolic)
            IOUtils.copy(new FileInputStream(file), taos);
        taos.closeArchiveEntry();
    }


    //将tar文件srcFile解包在当前目录下
    public static void dearchive(File srcFile) throws Exception {
        String basePath = srcFile.getParent();
        dearchive(srcFile, new File(basePath));
    }


    public static void dearchive(File srcFile, File destFile) throws Exception {

        TarArchiveInputStream tais = new TarArchiveInputStream(
                new FileInputStream(srcFile));
        dearchive(destFile, tais);

        tais.close();

    }



    private static void dearchive(File destFile, TarArchiveInputStream tais)
            throws Exception {

        TarArchiveEntry entry = null;
        while ((entry = tais.getNextTarEntry()) != null) {

            // 文件
            String dir = destFile.getPath() + File.separator + entry.getName();

            File dirFile = new File(dir);

            // 文件检查
            fileProber(dirFile);

            if (entry.isDirectory()) {
                dirFile.mkdirs();
            } else if (entry.isSymbolicLink()) {
                File target = new File(entry.getLinkName());
                Files.createSymbolicLink(dirFile.toPath(), target.toPath());
            } else {
                boolean executable = (entry.getMode() % 2) == 1;
                dearchiveFile(dirFile, tais);
                dirFile.setExecutable(executable);
            }

        }
    }


    public static void dearchive(String srcPath) throws Exception {
        File srcFile = new File(srcPath);

        dearchive(srcFile);
    }




    private static void dearchiveFile(File destFile, TarArchiveInputStream tais)
            throws Exception {

        BufferedOutputStream bos = new BufferedOutputStream(
                new FileOutputStream(destFile));

        int count;
        byte data[] = new byte[BUFFERSZ];
        while ((count = tais.read(data, 0, BUFFERSZ)) != -1) {
            bos.write(data, 0, count);
        }

        bos.close();
    }


    private static void fileProber(File dirFile) {

        File parentFile = dirFile.getParentFile();
        if (!parentFile.exists()) {

            // 递归寻找上级目录
            fileProber(parentFile);

            parentFile.mkdir();
        }

    }

}