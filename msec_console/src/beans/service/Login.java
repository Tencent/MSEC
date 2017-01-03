
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
import beans.response.LoginResponse;
import ngse.org.*;

import java.security.SecureRandom;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

/**
 * Created by Administrator on 2016/2/10.
 * 用户登录
 */
public class Login extends JsonRPCHandler {

    public static String userTicketKey = initUserTicketKey();

    private static String initUserTicketKey()
    {
        SecureRandom rnd = new SecureRandom();
        char[] chars = {'0', '1','2','3', '4','5','6','7','8','9','a','b','c','d','e','f'};
        int i;
        StringBuffer result = new StringBuffer();
        for (i = 0; i < 32; ++i)
        {
            int index = Math.abs(rnd.nextInt()) % 16;
            result.append(chars[index]);
        }
        System.out.println(">>>"+result);
        return result.toString();

    }

    public static String geneTicket(String userName)
    {

        String userNameMd5 =  Tools.md5(userName);//32bytes
        String dt = String.format("%16d", new Date().getTime()/1000);


        String plainText = userNameMd5+dt; // 48bytes

        JsTea tea = new JsTea(null);
        return tea.encrypt(plainText, Login.userTicketKey);//96bytes
    }
    static public  boolean  checkTicket(String userName, String ticket)
    {
        if (ticket.length() != 96) { return false;}

        try {

            JsTea tea = new JsTea(null);
            String s = tea.decrypt(ticket, Login.userTicketKey);
            if (s.length() != 48) {
                return false;
            }
            String userNameMd5 = s.substring(0, 32);
            String dt = s.substring(32, 48);
            long ticketInitTime = new Integer(dt.trim()).intValue();
            long currentTime = new Date().getTime() / 1000;
            if (ticketInitTime < currentTime && (currentTime - ticketInitTime) > (3600 * 24)) {
                return false;
            }
            String md5Str = Tools.md5(userName);
            if (md5Str.equals(userNameMd5)) {
                return true;
            } else {
                return false;
            }
        }
        catch (Exception e)
        {
            return false;
        }

    }


     public LoginResponse exec(LoginRequest request)
     {
         LoginResponse resp = new LoginResponse();

         if (request.getStaff_name() == null && request.getTgt() == null)
         {
             resp.setStatus(100);
             resp.setMessage("login name /password empty!");
             return resp;
         }



         DBUtil util = new DBUtil();
         if (util.getConnection() == null)
         {
             resp.setStatus(100);
             resp.setMessage("db connect failed!");
             return resp;
         }
         List<StaffInfo> staffInfoList ;


         String sql = "select staff_name, staff_phone,password,salt from t_staff where  staff_name=? ";
         List<Object> params = new ArrayList<Object>();
         params.add(request.getStaff_name());

         try {
             staffInfoList = util.findMoreRefResult(sql, params, StaffInfo.class);
             if (staffInfoList.size() != 1)
             {
                 resp.setMessage("user does NOT exist.");
                 resp.setStatus(100);
                 return resp;
             }
             //用加盐的二次密码hash作为key（数据库里存着）解密
             StaffInfo staffInfo = staffInfoList.get(0);
             JsTea tea = new JsTea(this.getServlet());
             String p1 = tea.decrypt(request.getTgt(), staffInfo.getPassword());
             ///获取session里保存的challenge
             String challenge = (String)(getHttpRequest().getSession().getAttribute(GetSalt.CHALLENGE_KEY_IN_SESSION));
             if (p1.length() != 40 )
             {
                 resp.setMessage("password error!");
                 resp.setStatus(100);
                 return resp;
             }
             //看解密处理的后面部分内容是否同challenge，放重放
             if (!p1.substring(32).equals(challenge))
             {
                 resp.setMessage("password error!!");
                 resp.setStatus(100);
                 return resp;
             }
            //根据解密出来的一次密码hash，现场生成二次加盐的hash，与数据库里保存的比较看是否相等
             String p2 = AddNewStaff.geneSaltedPwd(p1.substring(0, 32), staffInfo.getSalt());
             if (!p2.equals(staffInfo.getPassword()))
             {
                 resp.setMessage("password error!!!");
                 resp.setStatus(100);
                 return resp;
             }
             String ticket = "";
             resp.setStaff_name(request.getStaff_name());
             resp.setTicket(geneTicket(request.getStaff_name()));
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
