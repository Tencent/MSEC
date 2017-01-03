
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


package beans.dbaccess;

import ngse.org.ServletConfig;

import java.io.File;

/**
 * Created by Administrator on 2016/1/28.
 */
public class IDL {
    String tag_name;
    String memo;
    String content;
    String first_level_service_name;
    String second_level_service_name;

    public static String getIDLFileDirectory()
    {
        String rootdir = ServletConfig.fileServerRootDir;
        String filename = rootdir+ File.separator+"IDLFile";

        return filename;
    }

    public static String getIDLFileName(String flsn, String slsn, String tag)
    {

        String filename = getIDLFileDirectory()+File.separator+flsn+"."+slsn+"."+tag+".idl";
        return filename;
    }

    public String getTag_name() {
        return tag_name;
    }

    public void setTag_name(String tag_name) {
        this.tag_name = tag_name;
    }

    public String getMemo() {
        return memo;
    }

    public void setMemo(String memo) {
        this.memo = memo;
    }

    public String getContent() {
        return content;
    }

    public void setContent(String content) {
        this.content = content;
    }

    public String getFirst_level_service_name() {
        return first_level_service_name;
    }

    public void setFirst_level_service_name(String first_level_service_name) {
        this.first_level_service_name = first_level_service_name;
    }

    public String getSecond_level_service_name() {
        return second_level_service_name;
    }

    public void setSecond_level_service_name(String second_level_service_name) {
        this.second_level_service_name = second_level_service_name;
    }
}
