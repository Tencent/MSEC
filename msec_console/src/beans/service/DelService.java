
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

import beans.dbaccess.DBAnalyzeInfo;
import beans.request.DelServiceRequest;
import beans.response.DelServiceResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import org.apache.log4j.Logger;

/**
 * Created by Administrator on 2016/1/27.
 * 删除标准服务
 */
public class DelService extends JsonRPCHandler{

    private boolean checkIfHasSecondLevel(String first_level_service_name, DBUtil util) throws Exception
    {
       String sql = "select count(*) as record_number from t_second_level_service where first_level_service_name=?";
        List<Object> params = new ArrayList<Object>();
        params.add(first_level_service_name);

        DBAnalyzeInfo dbinfo = util.findSimpleRefResult(sql, params, DBAnalyzeInfo.class);
        if (dbinfo.getRecord_number() > 0) {
            return true;
        } else {
            return false;
        }

    }
    private boolean checkIfOnService(String flsn, String slsn, DBUtil util) throws Exception
    {
        String sql = "select count(*) as record_number from t_second_level_service_ipinfo where first_level_service_name=?" +
                " and second_level_service_name=? and status='enabled'";
        List<Object> params = new ArrayList<Object>();
        params.add(flsn);
        params.add(slsn);

        DBAnalyzeInfo dbinfo = util.findSimpleRefResult(sql, params, DBAnalyzeInfo.class);
        if (dbinfo.getRecord_number() > 0) {
            return true;
        } else {
            return false;
        }

    }


    public DelServiceResponse exec(DelServiceRequest request)
    {
        Logger logger = Logger.getLogger(this.getClass().getName());
    DelServiceResponse response = new DelServiceResponse();


    response.setMessage("unkown error.");
    response.setStatus(100);
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
    if (request.getService_name() == null ||
            request.getService_name().equals("") ||
            request.getService_level() == null ||
            request.getService_level().equals(""))
    {
        response.setMessage("The name/level of service to be deled should NOT be empty.");
        response.setStatus(100);
        return response;
    }
    if ( request.getService_level().equals("second_level") &&
            ( request.getService_parent() == null || request.getService_parent().equals("")))
    {
        response.setMessage("The first level service name  should NOT be empty.");
        response.setStatus(100);
        return response;
    }
    DBUtil util = new DBUtil();
    if (util.getConnection() == null)
    {
        response.setMessage("DB connect failed.");
        response.setStatus(100);
        return response;
    }
    String sql;
    List<Object> params = new ArrayList<Object>();



    try {
        if (request.getService_level().equals("first_level")) {
            //检查一下一级服务下还有没有二级服务，有的话不能删除
            if (checkIfHasSecondLevel(request.getService_name(), util) != false)
            {
                response.setMessage("还有二级服务挂靠在该一级服务下，不能删除该服务.");
                response.setStatus(100);
                return response;
            }

            sql = "delete from t_first_level_service where first_level_service_name=?";
            logger.info(sql);
            params.add(request.getService_name());

        }
        else
        {
            if (checkIfOnService(request.getService_parent(), request.getService_name(), util) != false)
            {
                response.setMessage("还有IP在该二级服务下提供服务。请先缩容后删除服务。");
                response.setStatus(100);
                return response;
            }

            sql = "delete from t_second_level_service where second_level_service_name=? and first_level_service_name=?";
            logger.info(sql);
            params.add(request.getService_name());
            params.add(request.getService_parent());
        }
        int delNum = util.updateByPreparedStatement(sql, params);
        if (delNum == 1)
        {
            //相关的一些信息也应该删除掉，例如IP、config、IDL等等
            if (request.getService_level().equals("second_level")) {
                DelIDLTag.deleteAll(request.getService_parent(), request.getService_name());
                DelSecondLevelServiceConfigTag.deleteAll(request.getService_parent(), request.getService_name());
                DelSecondLevelServiceIPInfo.deleteAll(request.getService_parent(), request.getService_name());
                DelLibraryFile.deleteAll(request.getService_parent(), request.getService_name());
                DelSharedobject.deleteAll(request.getService_parent(), request.getService_name());
            }
            response.setDelNum(delNum);
            response.setMessage("success");
            response.setStatus(0);
            return response;
        }
        else
        {
            response.setDelNum(delNum);
            response.setMessage("delete record number is "+delNum);
            response.setStatus(100);
            return response;
        }

    }
    catch (Exception e)
    {
        response.setMessage("exception:"+e.toString());
        response.setStatus(100);
        e.printStackTrace();
        return response;
    }
    finally {
        util.releaseConn();
    }

}
}
