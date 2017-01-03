
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

import beans.dbaccess.*;
import beans.request.QuerySecondLevelServiceDetailRequest;
import beans.response.QueryFirstLevelServiceResponse;
import beans.response.QuerySecondLevelServiceDetailResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/27.
 * 查询某个二级标准服务的详情
 */
public class QuerySecondLevelServiceDetail extends JsonRPCHandler {
    public QuerySecondLevelServiceDetailResponse exec(QuerySecondLevelServiceDetailRequest request)
    {
        QuerySecondLevelServiceDetailResponse resp = new QuerySecondLevelServiceDetailResponse();
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            resp.setStatus(99);
            resp.setMessage(result);
            return resp;
        }
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            resp.setStatus(100);
            resp.setMessage("db connect failed!");
            return resp;
        }



        String sql;
        List<Object> params;
        List<SecondLevelService> serviceList ;
        List<SecondLevelServiceIPInfo> ipList;
        List<SecondLevelServiceConfigTag> tagList;
        List<IDL> IDLList;
        List<LibraryFile> libraryFileList;
        List<SharedobjectTag> sharedobjectTagList;
        try {
            //查出开发语言 port

            sql = "select dev_lang,port from t_second_level_service where second_level_service_name=?  and first_level_service_name=?";
            params = new ArrayList<Object>();
            params.add(request.getService_name());
            params.add(request.getService_parent());

            serviceList = util.findMoreRefResult(sql, params, SecondLevelService.class);
            if (serviceList.size() != 1)
            {
                resp.setStatus(100);
                resp.setMessage("can not find the service named "+request.getService_name());
                return resp;
            }
            //resp.setFirst_level_service_name(serviceList.get(0).getFirst_level_service_name());
            resp.setDev_lang(serviceList.get(0).getDev_lang());
            resp.setPort(serviceList.get(0).getPort());

            resp.setFirst_level_service_name(request.getService_parent());
            //查出ip列表
            sql = "select ip,port,status,second_level_service_name,first_level_service_name, release_memo,comm_proto from " +
                    "t_second_level_service_ipinfo where second_level_service_name=? and first_level_service_name=?";
            params = new ArrayList<Object>();
            params.add(request.getService_name());
            params.add(resp.getFirst_level_service_name());

            ipList = util.findMoreRefResult(sql, params, SecondLevelServiceIPInfo.class);

            resp.setIpList((ArrayList<SecondLevelServiceIPInfo>) ipList);
            //查出配置文件的tag列表
            sql = "select tag_name, memo,second_level_service_name,first_level_service_name from t_config_tag where second_level_service_name=? and first_level_service_name=?";
            params = new ArrayList<Object>();
            params.add(request.getService_name());
            params.add(resp.getFirst_level_service_name());

            tagList = util.findMoreRefResult(sql, params, SecondLevelServiceConfigTag.class);

            resp.setConfigTagList((ArrayList<SecondLevelServiceConfigTag>) tagList);

            //查出 IDL文件的tag列表
            sql = "select tag_name, memo,second_level_service_name,first_level_service_name from t_idl_tag where second_level_service_name=? and first_level_service_name=?";
            params = new ArrayList<Object>();
            params.add(request.getService_name());
            params.add(resp.getFirst_level_service_name());

            IDLList = util.findMoreRefResult(sql, params, IDL.class);

            resp.setIDLTagList((ArrayList<IDL>) IDLList);

            //查出 library文件列表
            sql = "select file_name, memo,second_level_service_name,first_level_service_name from t_library_file where second_level_service_name=? and first_level_service_name=?";
            params = new ArrayList<Object>();
            params.add(request.getService_name());
            params.add(resp.getFirst_level_service_name());

            libraryFileList = util.findMoreRefResult(sql, params, LibraryFile.class);
            resp.setLibraryFileList((ArrayList<LibraryFile>) libraryFileList);

            //查出 业务插件的tag列表
            sql = "select tag_name, memo,second_level_service_name,first_level_service_name from t_sharedobject_tag where second_level_service_name=? and first_level_service_name=?";
            params = new ArrayList<Object>();
            params.add(request.getService_name());
            params.add(resp.getFirst_level_service_name());

            sharedobjectTagList = util.findMoreRefResult(sql, params, SharedobjectTag.class);
            resp.setSharedobjectTagList((ArrayList<SharedobjectTag>)sharedobjectTagList);



            resp.setMessage("success");
            resp.setStatus(0);
            return resp;

        }
        catch (Exception e)
        {
            resp.setStatus(100);
            resp.setMessage("db query exception!");
            e.printStackTrace();
            return resp;
        }
        finally {
            util.releaseConn();
        }


    }
}
