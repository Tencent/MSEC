
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

import beans.request.ReleasePlan;
import beans.response.DelReleasePlanResponse;
import beans.response.DelServiceResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import org.apache.log4j.Logger;

/**
 * Created by Administrator on 2016/2/3.
 * 删除发布计划
 */
public class DelReleasePlan extends JsonRPCHandler {
    public DelReleasePlanResponse exec(ReleasePlan request)
    {
        Logger logger = Logger.getLogger(this.getClass().getName());
        DelReleasePlanResponse response = new DelReleasePlanResponse();


        response.setMessage("unkown error.");
        response.setStatus(100);
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getPlan_id() == null ||
                request.getPlan_id().equals(""))
        {
            response.setMessage("Plan ID  should NOT be empty.");
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

        sql = "delete from t_release_plan where plan_id=?";
        logger.info(sql);
        params.add(request.getPlan_id());

        try {
            int delNum = util.updateByPreparedStatement(sql, params);
            if (delNum == 1)
            {
                response.setDelNumber(delNum);
                response.setMessage("success");
                response.setStatus(0);
                return response;
            }
            else
            {
                response.setDelNumber(delNum);
                response.setMessage("delete record number is "+delNum);
                response.setStatus(100);
                return response;
            }
        }
        catch (SQLException e)
        {
            response.setMessage("del record failed:"+e.toString());
            response.setStatus(100);
            e.printStackTrace();
            return response;
        }
        finally {
            util.releaseConn();
        }

    }
}
