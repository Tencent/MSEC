
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

import beans.dbaccess.MachineInfo;
import beans.request.QueryMachineListRequest;
import beans.response.QueryMachineListResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/26.
 * 查询开发机编译机
 */
public class QueryMachineList extends JsonRPCHandler {
    
    public QueryMachineListResponse exec(QueryMachineListRequest request)
    {
        QueryMachineListResponse resp = new QueryMachineListResponse();
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
        List<MachineInfo> machineInfoList ;
        //System.out.printf("name:%s, ip:%s\n", request.getMachine_name(), request.getMachine_ip());

        String sql = "select machine_name, machine_ip, os_version, gcc_version, java_version from t_machine ";
        List<Object> params = new ArrayList<Object>();
        if (request.getMachine_name() != null && request.getMachine_name().length() > 0)
        {
            sql += " where machine_name=? ";
            params.add(request.getMachine_name());
        }
        else if (request.getMachine_ip() != null && request.getMachine_ip().length() > 0)
        {
            sql += " where machine_ip=? ";
            params.add(request.getMachine_ip());
        }
        try {
            machineInfoList = util.findMoreRefResult(sql, params, MachineInfo.class);

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



        resp.setMachine_list((ArrayList<MachineInfo>)machineInfoList);
        resp.setMessage("success");
        resp.setStatus(0);

        return resp;
    }
}
