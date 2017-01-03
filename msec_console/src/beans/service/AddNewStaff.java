
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

import beans.request.AddNewStaffRequest;
import beans.response.AddNewStaffResponse;
import beans.response.DelStaffResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import ngse.org.Tools;
import org.apache.log4j.Logger;

import java.security.MessageDigest;
import java.security.SecureRandom;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Created by Administrator on 2016/1/26.
 * 增加新用户
 */
public class AddNewStaff extends JsonRPCHandler {

    static public  String newSalt()//新生成密码加盐
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
        return sb.toString();
    }

    //生成加盐的密码hash
    static public  String geneSaltedPwd(String pwdOneTime, String salt)
    {
        if (pwdOneTime.length() != 32 || salt.length() != 8)
        {
            return "";
        }
        try {
            MessageDigest md = MessageDigest.getInstance("MD5");
            String s = pwdOneTime+salt;
            md.update(s.getBytes());
            byte[] result = md.digest();
            if (result.length != 16)
            {
                return "";
            }

            return Tools.toHexString(result);
        }
        catch (Exception e )
        {
            return "";
        }
    }

    public AddNewStaffResponse exec(AddNewStaffRequest request)
    {
        Logger logger = Logger.getLogger("AddNewStaff");
        AddNewStaffResponse response = new AddNewStaffResponse();
        response.setMessage("unkown error.");
        response.setStatus(100);

        //检查用户身份
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getStaff_name() == null ||
                request.getStaff_name().equals("") ||
                request.getStaff_phone() == null ||
                request.getStaff_phone().equals("")||
                request.getPassword() == null ||
                request.getPassword().equals(""))
        {
            response.setMessage("input should NOT be empty.");
            response.setStatus(100);
            return response;
        }

        //连接并插入数据库
        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setMessage("DB connect failed.");
            response.setStatus(100);
            return response;
        }


        String salt = AddNewStaff.newSalt();
        logger.info(String.format("p1:%s\nsalt:%s", request.getPassword(), salt));
        String saltedPwd = AddNewStaff.geneSaltedPwd(request.getPassword(), salt);

        if (saltedPwd.equals("")) {
            response.setMessage("geneSaltedPwd() failed.");
            response.setStatus(100);
            return response;
        }
        logger.info(String.format("p2:%s", saltedPwd));
        String sql = "insert into t_staff(staff_name, staff_phone, password, salt) values(?,?,?, ?)";
        List<Object> params = new ArrayList<Object>();
        params.add(request.getStaff_name());
        params.add(request.getStaff_phone());
        params.add(saltedPwd);
        params.add(salt);

        try {
            int addNum = util.updateByPreparedStatement(sql, params);
            if (addNum >= 0)
            {
                response.setMessage("success");
                response.setStatus(0);
                return response;
            }
        }
        catch (SQLException e)
        {
            response.setMessage("add record failed:"+e.toString());
            response.setStatus(100);
            e.printStackTrace();
            return response;
        }
        finally {
            util.releaseConn();
        }
        return  response;
    }
}
