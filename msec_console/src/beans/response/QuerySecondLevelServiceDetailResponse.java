
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


package beans.response;

import beans.dbaccess.*;
import ngse.org.JsonRPCResponseBase;

import java.util.ArrayList;

/**
 * Created by Administrator on 2016/1/27.
 */
public class QuerySecondLevelServiceDetailResponse extends JsonRPCResponseBase {
    ArrayList<SecondLevelServiceIPInfo> ipList;
    ArrayList<SecondLevelServiceConfigTag> configTagList;
    ArrayList<IDL> IDLTagList;
    ArrayList<LibraryFile> libraryFileList;
    ArrayList<SharedobjectTag> sharedobjectTagList;
    String first_level_service_name;
    String dev_lang;
    int port;

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        this.port = port;
    }

    public String getDev_lang() {
        return dev_lang;
    }

    public void setDev_lang(String dev_lang) {
        this.dev_lang = dev_lang;
    }

    public ArrayList<SharedobjectTag> getSharedobjectTagList() {
        return sharedobjectTagList;
    }

    public void setSharedobjectTagList(ArrayList<SharedobjectTag> sharedobjectTagList) {
        this.sharedobjectTagList = sharedobjectTagList;
    }

    public ArrayList<LibraryFile> getLibraryFileList() {
        return libraryFileList;
    }

    public void setLibraryFileList(ArrayList<LibraryFile> libraryFileList) {
        this.libraryFileList = libraryFileList;
    }

    public ArrayList<IDL> getIDLTagList() {
        return IDLTagList;
    }


    public void setIDLTagList(ArrayList<IDL> IDLTagList) {
        this.IDLTagList = IDLTagList;
    }

    public ArrayList<SecondLevelServiceConfigTag> getConfigTagList() {
        return configTagList;
    }

    public void setConfigTagList(ArrayList<SecondLevelServiceConfigTag> configTagList) {
        this.configTagList = configTagList;
    }

    public ArrayList<SecondLevelServiceIPInfo> getIpList() {
        return ipList;
    }

    public void setIpList(ArrayList<SecondLevelServiceIPInfo> ipList) {
        this.ipList = ipList;
    }

    public String getFirst_level_service_name() {
        return first_level_service_name;
    }

    public void setFirst_level_service_name(String first_level_service_name) {
        this.first_level_service_name = first_level_service_name;
    }
}
