
<%--
  Tencent is pleased to support the open source community by making MSEC available.
 
  Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 
  Licensed under the GNU General Public License, Version 2.0 (the "License"); 
  you may not use this file except in compliance with the License. You may 
  obtain a copy of the License at
 
      https://opensource.org/licenses/GPL-2.0
 
  Unless required by applicable law or agreed to in writing, software distributed under the 
  License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
  either express or implied. See the License for the specific language governing permissions
  and limitations under the License.
--%>


<%@ page import="msec.org.DBUtil" %>
<%@ page import="java.util.List" %>
<%@ page import="java.util.ArrayList" %>
<%@ page import="beans.dbaccess.FirstLevelService" %>
<%@ page import="beans.dbaccess.SecondLevelService" %>
<%@ page import="beans.service.Login" %>
<%@ page import="java.io.PrintWriter" %>
<%@ page import="java.util.Map" %>
<%@ page import="java.util.HashMap" %>
<%@ page import="java.net.URLEncoder" %>
<%@ page import="java.nio.charset.Charset" %>
<%@ page import="com.sun.xml.internal.ws.transport.http.server.ServerAdapterList" %>
<%@ page import="beans.service.ServiceList" %>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>

<html>
<head>
  <title>
  </title>
  <link rel="stylesheet" type="text/css" href="/css/MenuTree.css"/>

  <script type="text/javascript" src="/js/MenuTree.js"></script>
  <script type="text/javascript">
    $(function(){
      $(".menu_tree").MenuTree({
        click:function(a){

          if ( $(a).attr("ref") == "leaf" || $(a).attr("ref") =="action" )
          {
            var dest = $(a).attr("href");
              $("#right").html("");
              $("#right").load(dest);


          }
        }
      });




    });



  </script>

    <script language="JavaScript" type="text/javascript">

        function reloadTree()
        {
            alert('hello');
            //$("#left").load("/pages/LeftMenu.jsp")
        }
    </script>

    <%
        if (!checkIdentity(request).equals("success"))
        {
            PrintWriter outf = response.getWriter();

            outf.printf("<script type=\"text/javascript\">");
            outf.printf("document.location.href = \"/pages/users/login.jsp\";");
            outf.printf("</script>");


        }
    %>
</head>
<body>
<%! public   String printService()
  {
      StringBuffer outstr = new StringBuffer();
      Map<String, List<String>> slsn = new HashMap<String, List<String>>();
      List<String> flsn = new ArrayList<>();

      /*
      flsn.add("oidb");
      flsn.add("qzone");
      slsn.put("oidb", new ArrayList<String>());
      slsn.put("qzone", new ArrayList<String>());
      slsn.get("oidb").add("sns server");
      slsn.get("oidb").add("udc server");
      slsn.get("qzone").add("interface");
      */
      try {
          ServiceList.getServiceList(flsn, slsn);
      }
      catch (Exception e )
      {
          e.printStackTrace();
          return "";
      }


      try {

          for (int i = 0; i < flsn.size(); ++i)
          {


              outstr.append("<li><a href=\"#\" ref=\"\">" + flsn.get(i) + "</a></li>\r\n");

              List<String> sub_list = slsn.get(flsn.get(i));
              if (sub_list.size() > 0) {
                  outstr.append("<ul>\r\n");
                  for (int j = 0; j < sub_list.size(); j++) {


                      String svc_name = URLEncoder.encode(sub_list.get(j), "UTF-8");
                      String parent_name = URLEncoder.encode(flsn.get(i), "UTF-8");
                      outstr.append("<li><a href=\"/pages/monitor/Monitor.jsp?service=" + svc_name + "&service_parent="+parent_name+"\" ref=\"leaf\">" + sub_list.get(j) + "</a></li>\r\n");

                  }
                  outstr.append("</ul>\r\n");

              }

          }
          return outstr.toString();
      } catch (Exception e) {
          e.printStackTrace();
          return "";
      }

  }
%>

<%! public   String checkIdentity(HttpServletRequest req)
{
    Cookie[] cookies = req.getCookies();
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


%>






<div class="menu_tree">
    <ul>
      <li><a href="#" ref="">业务树</a></li>
        <ul>

            <%=printService()%>
        </ul>


<!--这部分是静态的-->

        <li><a href="#" ref="">信息管理</a></li>
        <ul>
            <li><a href="/pages/ShowDevOp.jsp" ref="leaf">用户</a></li>
            <li><a href="/pages/users/ChangePasswd.jsp" ref="leaf">改密</a></li>
        </ul>
        <li><a href="#" ref="">帮助</a></li>
        <ul>
            <li><a href="/pages/about.jsp" ref="leaf">关于</a></li>
            <li><a href="/pages/document.jsp" ref="leaf">文档</a></li>
        </ul>

</ul>
</div>

</body>
</html>
