
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

import beans.dbaccess.StaffInfo;
import beans.request.LoginRequest;
import beans.response.GetSaltResponse;
import beans.response.LoginResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;

import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/2/11.
 * 用户登录的时候，先获取服务器上该用户的盐，并返回一个挑战数，用于防止重放登录
 * 登录过程中用到了 session来保存过程中的上下文
 */
public class GetSalt extends JsonRPCHandler {
    public static String CHALLENGE_KEY_IN_SESSION="login_challenge";
    private String geneChallenge()//一个随机数
    {
        SecureRandom rnd = new  java.security.SecureRandom();
        int i;
        StringBuffer sb = new StringBuffer();
        char[] chars = {'0', '1','2','3', '4','5','6','7','8','9','a','b','c','d','e','f'};
        for (i = 0; i < 8; ++i)
        {
            int index = (int)( rnd.nextDouble()*16);
            sb.append(chars[index]);
        }
        String challenge = sb.toString();
        getHttpRequest().getSession().setAttribute(GetSalt.CHALLENGE_KEY_IN_SESSION, challenge);
        return  challenge;
    }
    public GetSaltResponse exec(LoginRequest request)
    {
        GetSaltResponse resp = new GetSaltResponse();

        if (request.getStaff_name() == null || request.getStaff_name().length() < 1)
        {
            resp.setStatus(100);
            resp.setMessage("login name empty!");
            return resp;
        }
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            resp.setStatus(100);
            resp.setMessage("db connect failed!");
            return resp;
        }
        List<StaffInfo> saltList ;


        String sql = "select salt from t_staff where  staff_name=? ";
        List<Object> params = new ArrayList<Object>();
        params.add(request.getStaff_name());

        try {
            saltList = util.findMoreRefResult(sql, params, StaffInfo.class);
            if (saltList.size() != 1)
            {
                resp.setMessage("query salt failed");
                resp.setStatus(100);
                return resp;
            }
            String salt = saltList.get(0).getSalt();
            String challenge = geneChallenge();
            resp.setMessage("success");
            resp.setChallenge(challenge);
            resp.setSalt(salt);
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
