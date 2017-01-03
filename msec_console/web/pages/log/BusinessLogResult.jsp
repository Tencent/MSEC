
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


<%@ page import="beans.dbaccess.BusinessLogResult" %>
<%@ page import="java.util.ArrayList" %>
<%@ page import="java.util.HashMap" %>
<%@ page import="java.io.PrintWriter" %>
<%@ page import="java.net.URLEncoder" %>
<%--
  Created by IntelliJ IDEA.
  User: Administrator
  Date: 2016/1/25
  Time: 14:07
  To change this template use File | Settings | File Templates.
--%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<html>
<head>
    <title></title>
    <link rel="stylesheet" type="text/css" href="/css/MenuTree.css"/>
    <link rel="stylesheet" type="text/css" href="/css/bootstrap.min.css"/>
    <link rel="stylesheet" href="/css/main.css">
    <link rel="stylesheet" href="/js/jquery_ui/jquery-ui.css">
    <link rel="stylesheet" href="/js/jquery_ui/jquery-ui.structure.css">
    <link rel="stylesheet" href="/js/jquery_ui/jquery-ui.theme.css">

    <script type="text/javascript" src="/js/jquery-2.2.0.min.js"></script>
    <script type="text/javascript" src="/js/jquery.form.js"></script>
    <script type="text/javascript" src="/js/jquery.cookie.js"></script>
    <script type="text/javascript" src="/js/jquery_ui/jquery-ui.js"></script>




  <script type="text/javascript">



      function onResultColumnToggleBtnClicked()
      {
          $("#div_result_column").toggle();


          if ($("#div_result_column").is(":hidden"))
          {
              $("#btn_toggle_result_column").text('+');
          }
          else
          {
              $("#btn_toggle_result_column").text('-');
          }
      }
      function ToggleColumn()
      {
          var items = document.getElementsByName("result_column");
          for (i = 0; i < items.length; ++i)
          {

             var name = items[i].value;
              var columns = document.getElementsByName( name);
              if (items[i].checked)
              {
                    for (j = 0; j < columns.length; ++j)
                    {
                        columns[j].hidden = false;
                    }
              }
              else
              {
                  for (j = 0; j < columns.length; ++j)
                  {
                      columns[j].hidden = true;;
                  }
              }
          }
      }

      $(document).ready(function() {

         $("#IP").attr("checked", false);
          $("#ClientIP").attr("checked", false);
          $("#ServerIP").attr("checked", false);
          $("#Level").attr("checked", false);
          $("#FileLine").attr("checked", false);

          ToggleColumn();


      });



  </script>
</head>
<body>

<%

    BusinessLogResult result = (BusinessLogResult)request.getSession().getAttribute("BusinessLogResult");
    if (result == null)//如果没有结果，就搞个空的结果，至少后面的代码不会出异常
    {
        result = new BusinessLogResult();
        result.setLog_records(new ArrayList<HashMap<String, String>>());
        result.setColumn_names(new ArrayList<String>());
    }
    request.getSession().removeAttribute("BusinessLogResult");//这样能释放空间不？
    String callGraph = result.getCall_relation_graph();
    if (callGraph != null && !callGraph.equals(""))
    {

        String rqstr = String.format("{\"handleClass\":\"beans.service.DownloadCallGraph\",\"requestBody\": {\"request_id\":\"%s\"}}",
                                    callGraph);
        String imgurl = "/JsonRPCServlet?request_string="+URLEncoder.encode(rqstr, "UTF-8");





%>
    <div    align="middle">
        <img src="<%=imgurl%>"/>
    </div>
<%
    }
%>


<div  class="form_style">
 <form class="form-inline">
<button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_result_column" onclick="onResultColumnToggleBtnClicked()">+</button>
<label for="div_result_column"  >结果字段</label>
<div class="form-style" id="div_result_column" hidden="true">
    <!--ReqID, IP, ClientIP, ServerIP, Level, RPCName, ServiceName, FileLine, Function, instime-->

    <%
        ArrayList<String> column_names  = result.getColumn_names();
        for (int i = 0; i < column_names.size(); ++i)
        {
            String col_name = column_names.get(i);
    %>
    <input id="<%=col_name%>" type="checkbox" name="result_column" onclick="ToggleColumn()" value="<%=col_name%>" checked><%=col_name%>&nbsp;&nbsp;
    <%
        }
    %>
</div>
</form>
</div>

<div>
    <table border="1" width="100%" style="font-size: 8px">

<%
    out.println("<tr>");

    for (int i = 0; i < column_names.size(); ++i) {
        String col_name = column_names.get(i);
        out.println("<th name='"+col_name+"'>"+col_name+"</th>");

    }
    out.println("</tr>");
    ArrayList<HashMap<String, String>> log_records = result.getLog_records();
    for (int i = 0; i < log_records.size(); ++i)
    {
        HashMap<String, String> record = log_records.get(i);
        out.println("<tr>");
        for (int j = 0; j < column_names.size(); ++j)
        {
            String col_name = column_names.get(j);
            String col_value = (String)record.get(col_name);
            out.println("<td name='"+col_name+"'>"+col_value+"</td>\n");
        }

        out.println("</tr>");

    }
%>

    </table>
    </div>


</body>
</html>
