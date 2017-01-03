
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
import beans.response.QueryOddSecondLevelServiceDetailResponse;
import beans.response.QuerySecondLevelServiceDetailResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/27.
 * 查询异构的二级服务详情
 */
public class QueryOddSecondLevelServiceDetail extends JsonRPCHandler {
    public QueryOddSecondLevelServiceDetailResponse exec(OddSecondLevelService request)
    {
        QueryOddSecondLevelServiceDetailResponse resp = new QueryOddSecondLevelServiceDetailResponse();
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
        List<OddSecondLevelServiceIPInfo> ipList;

        try {

            //查出ip列表
            sql = "select ip,port,status,second_level_service_name,first_level_service_name,comm_proto from " +
                    "t_second_level_service_ipinfo where second_level_service_name=? and first_level_service_name=?";
            params = new ArrayList<Object>();
            params.add(request.getSecond_level_service_name());
            params.add(request.getFirst_level_service_name());

            ipList = util.findMoreRefResult(sql, params, OddSecondLevelServiceIPInfo.class);

            resp.setIpList((ArrayList<OddSecondLevelServiceIPInfo>) ipList);



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
