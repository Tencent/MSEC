
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
import beans.response.ReleaseStepsGOResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import org.codehaus.jackson.map.ObjectMapper;

import javax.servlet.http.HttpSession;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/2/1.
 * 制作发布计划的各个步骤的处理
 */
public class ReleaseOnlyConfig extends JsonRPCHandler  {


    private ReleaseStepsGOResponse doRelease(ReleasePlan request)
    {
        ReleaseStepsGOResponse response = new ReleaseStepsGOResponse();
        //把计划保存到session里
        HttpSession session = getHttpRequest().getSession();
        ReleasePlan plan = (ReleasePlan)session.getAttribute("plan");
        //System.out.println("plan id in session:"+plan.getPlan_id());
        if (plan == null)
        {
            response.setMessage("can NOT find the session, it maybe timeout.");
            response.setStatus(100);
            return response;
        }
        if (request.getConfig_tag() == null || request.getConfig_tag().length() < 1)
        {
            response.setMessage("config tag is empty!");
            response.setStatus(100);
            return response;
        }
        plan.setConfig_tag(request.getConfig_tag());
        plan.setMemo(request.getMemo());
        //check if saved
        ReleasePlan plan2 = (ReleasePlan)session.getAttribute("plan");
        //System.out.println("config tag:"+plan2.getConfig_tag());
        // Commit Plan
        String commitResult = CommitPlan(plan2);
        if (commitResult == null || !commitResult.equals("success"))
        {
            response.setMessage("commit failed:"+(commitResult== null ? "":commitResult));
            response.setStatus(100);
            return response;
        }
        response.setPlanDetail(plan2);

        response.setMessage("success");
        response.setStatus(0);
        return response;
    }


    private String CommitPlan(ReleasePlan plan)
    {
        //产生发布用的文件，并记录到数据库里
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            return "DB Connect Failed.";
        }
        String sql;
        List<Object> params = new ArrayList<Object>();

        try {
            ObjectMapper objectMapper = new ObjectMapper();
            String dest_ip_list_json_str = objectMapper.writeValueAsString(plan.getDest_ip_list());
            //System.out.println(dest_ip_list_json_str);

            sql = "insert into t_release_plan(plan_id, first_level_service_name,second_level_service_name," +
                    "config_tag,idl_tag, sharedobject_tag, dest_ip_list,status,memo,release_type) values(?,?,?,?,?,?,?,'creating',?,?)";
            params.add(plan.getPlan_id());
            params.add(plan.getFirst_level_service_name());
            params.add(plan.getSecond_level_service_name());
            params.add(plan.getConfig_tag());
            params.add("");
            params.add("");
            params.add(dest_ip_list_json_str);
            params.add(plan.getMemo());
            params.add(plan.getRelease_type());

            int addNum = util.updateByPreparedStatement(sql, params);
            if (addNum != 1)
            {
                return "addNum is not 1:"+addNum;
            }
            //打包文件
            new Thread(new PackReleaseFile(plan, getServlet().getServletContext())).start();



            return "success";
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
        finally {
            util.releaseConn();;
        }

    }

    public ReleaseStepsGOResponse exec(ReleasePlan request)
    {
        ReleaseStepsGOResponse response = new ReleaseStepsGOResponse();
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getSecond_level_service_name() == null || request.getSecond_level_service_name().length() < 1||
            request.getFirst_level_service_name() == null || request.getFirst_level_service_name().length() < 1   ||
                request.getPlan_id() == null || request.getPlan_id().length()<1)
        {
            response.setMessage("input is invalid");
            response.setStatus(100);
            return response;
        }

        return doRelease(request);

    }
}
