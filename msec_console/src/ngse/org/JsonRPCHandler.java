
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


package ngse.org;

import beans.service.Login;

import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/**
 * Created by Administrator on 2016/1/23.
 * 所有ajax后台处理类都继承自该类，该类有几个成员变量，保存了http请求的上下文信息
 * 方便业务流程处理的时候使用
 */
public class JsonRPCHandler {

    private HttpServlet servlet;
    private HttpServletRequest httpRequest;
    private HttpServletResponse httpResponse;

    public String checkIdentity()
    {
        Cookie[] cookies = this.getHttpRequest().getCookies();
        int i;
        String staffName = "";
        String ticket = "";
        for (i = 0; i < cookies.length; ++i)
        {
            Cookie cookie = cookies[i];
            if (cookie.getName().equals("msec_user"))
            {
                staffName = cookie.getValue();
            }
            if (cookie.getName().equals("msec_ticket"))
            {
                ticket = cookie.getValue();
            }
        }
        if (staffName.length() < 1 || ticket.length() < 1)
        {
            return "get cookie failed!";
        }
        if (Login.checkTicket(staffName, ticket))
        {
            return "success";
        }
        else
        {
            return "checkTicket() false";
        }

    }

    public HttpServlet getServlet() {
        return servlet;
    }

    public void setServlet(HttpServlet servlet) {
        this.servlet = servlet;
    }

    public HttpServletRequest getHttpRequest() {
        return httpRequest;
    }

    public void setHttpRequest(HttpServletRequest httpRequest) {
        this.httpRequest = httpRequest;
    }

    public HttpServletResponse getHttpResponse() {
        return httpResponse;
    }

    public void setHttpResponse(HttpServletResponse httpResponse) {
        this.httpResponse = httpResponse;
    }
}
