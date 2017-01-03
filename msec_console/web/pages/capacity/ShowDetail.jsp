
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


<%--
  Created by IntelliJ IDEA.
  User: Administrator
  Date: 2016/1/25
  Time: 14:07
  To change this template use File | Settings | File Templates.
--%>
<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<%
String service = request.getParameter("service_name");
String service_parent = request.getParameter("service_parent");
    %>
<html>
<head>
    <title></title>

  <style>
    #title_bar{width:auto;height:auto;   margin-top: 5px }
    #query_form{width:auto;height:auto;  padding:20px;margin-top: 5px;}
    #result_message{width:auto;height:auto;  margin-top: 5px ;}
    #result_table{width:auto;height:auto;  margin-top: 5px ;}
  </style>
    <script type="application/javascript" src="/js/ProgressTurn.js"/>
  <script type="text/javascript">

      var g_service_name = "<%=service%>";
      var g_service_parent = "<%=service_parent%>";

      function show_process_info()
      {
          var   request={
              "handleClass":"beans.service.QueryProcessInfo",
              "requestBody": {"first_level_service_name":g_service_parent,
                  "second_level_service_name":g_service_name,
              },
          };
          var url = "/JsonRPCServlet?request_string="+JSON.stringify(request);
          $("#process_info_div").load(url);
      }



      function show_detail()
      {


          var   request={
              "handleClass":"beans.service.QueryCapacityDetail",
              "requestBody": {"first_level_service_name":g_service_parent,
                    "second_level_service_name":g_service_name,
                    },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              var str = "<pre>";

                              if (data.info.length == 0) {
                                  str = "没有上报有效的负载数据.";

                              }
                              else {

                                  $.each(data.info, function (i, rec) {
                                      str += "ip:" + rec.ip + ", ";
                                      str += "负载:" + rec.load_level + "%\r\n";
                                  });
                                  str += "</pre>";
                              }

                              $("#result_table").empty();
                              $("#result_table").append(str);

                              $("#process_info_div").empty();
                              $("#process_info_div").append("正在查询该服务下机器的进程信息......");

                              show_process_info();


                          }
                          else if (data.status == 99)
                          {
                              document.location.href = "/pages/users/login.jsp";
                          }
                          else
                          {
                              //showTips(data.message);//业务处理失败的信息
                              var str="<p>"+data.message+"</p>";
                                showTips(str);
                          }
                      }
                      else
                      {
                          //showTips(status);//http通信失败的信息
                          var str="<p>"+status+"</p>";
                            showTips(str);
                      }

                  });

      }
      function change_capacity()
      {
          var url = "/pages/changeCapacitySteps/step1.jsp?service_name="+g_service_name+"&service_parent="+g_service_parent;
          console.log(url);
          $("#right").load(url);
      }
      function refresh_capacity()
      {


          var   request={
              "handleClass":"beans.service.RefreshCapacity",
              "requestBody": {"first_level_service_name":g_service_parent,
                  "second_level_service_name":g_service_name,
              },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功


                            showTips("刷新到LB成功");

                          }
                          else if (data.status == 99)
                          {
                              document.location.href = "/pages/users/login.jsp";
                          }
                          else
                          {
                              //showTips(data.message);//业务处理失败的信息
                              var str="<p>"+data.message+"</p>";
                              showTips(str);
                          }
                      }
                      else
                      {
                          //showTips(status);//http通信失败的信息
                          var str="<p>"+status+"</p>";
                          showTips(str);
                      }

                  });

      }



      $(document).ready(function() {
          show_detail();


      });




  </script>
</head>
<>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;容量详情</p>
</div>

<div id="query_form" class="form_style">

  <button type="button" class="btn-small" id="btn_query" onclick="change_capacity()">扩缩容</button>
    <button type="button" class="btn-small" id="btn_refresh" onclick="refresh_capacity()">重刷该服务到LB</button>



</div>
<div id="result_message" class="result_msg_style">
</div>


<div id="result_table" class="table_style">

    <div id="process_info_div" class="result_msg_style">

    </div>

  </div>
<div style="display:none;width:100px;margin:0 auto;position:fixed;left:45%;top:45%;" id="div_loading">
    <img src="/imgs/progress.gif" />
</div>
</body>
</html>
