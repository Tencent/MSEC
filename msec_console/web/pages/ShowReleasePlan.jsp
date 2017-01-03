
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
      g_service_relation = {};

      function query_service_list()
      {
          var   request={
              "handleClass":"beans.service.QuerySecondLevelServiceList",
              "requestBody": {},
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              g_service_relation = {};
                              var i;
                              for (i = 0; i < data.service_list.length; ++i)
                              {
                                  var first_name = data.service_list[i].first_level_service_name;
                                  var second_name = data.service_list[i].second_level_service_name;
                                  if (first_name in g_service_relation)
                                  {
                                      g_service_relation[first_name].push(second_name);
                                  }
                                  else
                                  {
                                      g_service_relation[first_name] = [second_name];
                                  }
                              }


                              var str = "";
                              str += "<option value=\"\">所有</option>";
                              for (prop in g_service_relation )
                              {
                                  str += "<option value=\""+prop+"\">"+prop+"</option>";
                              }
                              $("#first_level_service_name").empty();
                              $("#first_level_service_name").append(str);

                              var str = "";
                              str += "<option value=\"\">所有</option>";
                              $("#second_level_service_name").empty();
                              $("#second_level_service_name").append(str);


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
      function onFirstServiceSelectedChanged()
      {
          var first_name = $("#first_level_service_name").val();
          if (first_name != null && (first_name == "所有" || first_name == ""))
          {
              var str = "";
              str += "<option value=\"\">所有</option>";
              $("#second_level_service_name").empty();
              $("#second_level_service_name").append(str);
          }
          if ( first_name != null && first_name != "" && first_name != "所有" )
          {
              if (first_name in g_service_relation)
              {
                  var arr = g_service_relation[first_name];
                  var str = "";
                  //str += "<option value=\"\">所有</option>";
                  var i;
                  for (i = 0; i < arr.length; ++i)
                  {
                      var prop = arr[i];
                      str += "<option value=\""+prop+"\">"+prop+"</option>";
                  }
                  $("#second_level_service_name").empty();
                  $("#second_level_service_name").append(str);
              }
          }

      }

      function rollback_plan(plan_id, flsn, slsn)
      {
          if (!confirm('确定回滚吗？ '+plan_id+"?"))
          {
              return;
          }
          var   request={
              "handleClass":"beans.service.RollbackReleasePlan",
              "requestBody": {"plan_id": plan_id,
                  "first_level_service_name":flsn,
                  "second_level_service_name":slsn},
          };
          var url = "/JsonRPCServlet?request_string="+JSON.stringify(request);
          window.open(url);

      }
    function del_plan(plan_id)
    {
      if (!confirm('确定删除 '+plan_id+" 吗?"))
      {
        return;
      }
      var   request={
        "handleClass":"beans.service.DelReleasePlan",
        "requestBody": {"plan_id": plan_id},
      };
      $.post("/JsonRPCServlet",
              {request_string:JSON.stringify(request)},

              function(data, status){

                if (status == "success")// http通信返回200
                {
                   if (data.status==0) {
                     $("#table_row_" + plan_id).hide();
                       //table_row_160202101516.971952384
                   }
                   else if (data.status == 99)
                   {
                       document.location.href = "/pages/users/login.jsp";
                   }
                  else
                   {
                     showTips(data.message);
                   }
                }
                else
                {
                  showTips(status);
                }



              });
    }
      function plan_detail(plan_id)
      {
          url = "/pages/ShowPlanDetail.jsp?plan_id="+encodeURIComponent(plan_id);
        $("#right").load(url);
      }
      function onQueryPlanBtnClicked()
      {
          var   plan_id= $("#plan_id").val();
          var flsn = $("#first_level_service_name").val();
          var slsn = $("#second_level_service_name").val();

          var   request={
              "handleClass":"beans.service.QueryReleasePlan",
              "requestBody": {"plan_id":plan_id,
               "first_level_service_name":flsn,
                  "second_level_service_name":slsn
              },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              var str = "";

                              if (data.plan_list.length == 0)
                              {
                                  str="<p>no record match.</p>";
                                  $("#my_head").hide();

                                    showTips(str);
                                  $("#my_table").empty();
                                  return;
                              }

                              $("#my_head").show();

                              $.each(data.plan_list, function (i, rec) {


                                  //<td>Plan ID</td><td>一级业务</td><td>二级业务</td><td>备注</td><td>操作</td>
                                  str += "<tr id='table_row_" + rec.plan_id + "'>";
                                  str += "<td>" + rec.plan_id + "</td>";
                                  str += "<td>" + rec.first_level_service_name+"."+rec.second_level_service_name +"</td>";
                                  str += "<td>" + rec.memo+"</td>";
                                  str += "<td>" + rec.status+"</td>";
                                  str += "<td>";
                                  str +="<a href='#' onclick='del_plan(\"" + rec.plan_id + "\")'> 删除</a>";
                                  str +="<a href='#' onclick='plan_detail(\"" + rec.plan_id + "\")'> 详细</a>";

                                  if (rec.status == "carry out successfully" || rec.status == "failed to carry out" ){
                                      str += "<a href='#' onclick='rollback_plan(\"" + rec.plan_id;
                                      str += "\", \"" + rec.first_level_service_name;
                                      str += "\",\"" + rec.second_level_service_name;
                                      str += "\")'> 回滚</a>";
                                  }
                                  str+="</td>";
                                  str += "</tr>\n";
                              });

                              $("#my_table").empty();
                              $("#result_message").empty();
                              $(str).appendTo("#my_table");
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

          query_service_list();

      });




  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;发布计划</p>
</div>

<div id="query_form" class="form_style">
<form class="form-inline">
  <div class="form-group">

    <label for="plan_id">Plan ID:</label>
    <input type="text" class="form-control" id="plan_id" placeholder="支持%模糊查找" >
      <label for="first_level_service_name">一级服务:</label>
      <select class="form-control" id="first_level_service_name"   onchange="onFirstServiceSelectedChanged()">
      </select>
      <label for="second_level_service_name">二级服务:</label>
      <select class="form-control" id="second_level_service_name"  >
      </select>
  </div>

  <button type="button" class="btn-small" id="btn_query" onclick="onQueryPlanBtnClicked()">查询</button>


</form>

</div>
<div id="result_message" class="result_msg_style">
</div>

<div id="result_table" class="table_style">

<table class="table table-hover" >
  <thead id="my_head" hidden="true">
    <td>Plan ID</td><td>业务</td><td>备注</td><td>状态</td><td>操作</td>
  </thead>
  <tbody id="my_table">
  </tbody>
</table>
  </div>
</body>
</html>
