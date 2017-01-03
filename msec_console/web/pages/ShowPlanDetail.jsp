
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
    String g_plan_id = request.getParameter("plan_id");
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
  <script type="text/javascript">
      var g_plan_id ="<%=g_plan_id%>";
      var g_flsn = "";
      var g_slsn = "";
      var g_status = "";

      function do_release(plan_id)
      {
          if (g_status == "creating")
          {
              return;
          }
          if (!confirm("确定要发布吗？"))
          {
              return;
          }
          var   request={
              "handleClass":"beans.service.CarryOutReleasePlan",
              "requestBody": {"plan_id": g_plan_id,
                    "first_level_service_name":g_flsn,
                  "second_level_service_name":g_slsn
              },
          };
          var url = "/JsonRPCServlet?request_string="+JSON.stringify(request);
          window.open(url);


      }


      function show_plan_detail()
      {

          var   request={
              "handleClass":"beans.service.QueryReleasePlanDetail",
              "requestBody": {"plan_id":g_plan_id},
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              var str = "";


                                var rec = data.detail;

                                g_flsn = rec.first_level_service_name;
                                g_slsn = rec.second_level_service_name;
                              g_status = rec.status;



                                  str += "id:" + rec.plan_id + "<br>";

                                  str += "业务:&nbsp;&nbsp;" + rec.first_level_service_name+"."+rec.second_level_service_name +"<br>";
                                  str += "备注:&nbsp;&nbsp;" + rec.memo+"<br>";
                                  str += "<B>状态:&nbsp;&nbsp;" + rec.status+"</B><br>";
                                  str += "配置:&nbsp;&nbsp;" +rec.config_tag+"<br>";
                                  str += "接口定义:&nbsp;&nbsp;"+rec.idl_tag+"<br>";
                                  str += "业务so:&nbsp;&nbsp;"+rec.sharedobject_tag+"<br>";
                                  str += "安装包制作的详细信息:&nbsp;&nbsp;"+rec.backend_task_status+"<br>";
                                str += "ip list:<br>";
                                 var i;
                              for (i = 0; i < rec.dest_ip_list.length;++i)
                              {
                                  str += ""+rec.dest_ip_list[i].ip+":"+rec.dest_ip_list[i].port+"<br>";
                              }






                              $("#plan_detail").empty();
                              $("#plan_detail").append(str);


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
        show_plan_detail();
      });




  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;发布计划的详情</p>
</div>

<div class="form_style">

  <div class="form-group">

    <button type="button" class="btn-small" onclick="show_plan_detail()">刷新</button>
      <button id="btn_doRelease" type="button" class="btn-small" onclick="do_release()">发布</button>
      <p id="plan_detail"></p>

  </div>



</div>
<div id="result_message" class="result_msg_style">
</div>

</body>
</html>
