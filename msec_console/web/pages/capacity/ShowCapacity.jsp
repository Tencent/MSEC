
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

    <script type="application/javascript" src="/js/ProgressTurn.js"/>
  <script type="text/javascript">

      //一个对象，字段名是一级服务的名字，字段值是一个数组，里面保存了下面二级服务的名字列表
      //用于界面选择框修改的时候刷新可选的二级服务列表
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
                  str += "<option value=\"\">所有</option>";
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

      function onQueryCapacityListBtnClicked()
      {
          var   first_level_service_name= $("#first_level_service_name").val();
          var   second_level_service_name= $("#second_level_service_name").val();
          var   load_level= parseInt($("#load_level").val());

          var   request={
              "handleClass":"beans.service.QueryCapacityList",
              "requestBody": {"first_level_service_name":first_level_service_name,
                    "second_level_service_name":second_level_service_name,
                    "load_level":load_level},
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              var str = "";

                              if (data.capacity_list.length == 0)
                              {
                                  str="<p>no record match.</p>";
                                  $("#my_head").hide();

                                    showTips(str);
                                  $("#my_table").empty();
                                  return;
                              }
                              $("#my_head").show();

                              $.each(data.capacity_list, function (i, rec) {
                                  str += "<tr>";
                                  str += "<td>" + rec.first_level_service_name+":"+ rec.second_level_service_name+ "</td>";
                                  str += "<td>" +rec.load_level  + "</td><td>" + rec.ip_report_num+"/"+rec.ip_count+"</td>";
                                  str += "<td>";
                                  str +="<a href='javascript:change_capacity(\"" + rec.first_level_service_name+"\",\""+rec.second_level_service_name + "\")'> 扩缩容</a>";
                                  str +="&nbsp;&nbsp;<a href='javascript:show_detail(\"" + rec.first_level_service_name+"\",\""+rec.second_level_service_name + "\")'> 详细</a>";
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
      function change_capacity(first_name, second_name)
      {
          var url = "/pages/changeCapacitySteps/step1.jsp?service_name="+second_name+"&service_parent="+first_name;
          console.log(url);
          $("#right").load(url);
      }
      function show_detail(first_name, second_name)
      {
          var url = "/pages/capacity/ShowDetail.jsp?service_name="+second_name+"&service_parent="+first_name;
          console.log(url);
          $("#right").load(url);
      }


      $(document).ready(function() {

          query_service_list();

      });




  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;容量管理</p>
</div>

<div id="query_form" class="form_style">
<form class="form-inline">
  <div class="form-group">
    <label for="first_level_service_name">一级服务:</label>
    <select class="form-control" id="first_level_service_name"   onchange="onFirstServiceSelectedChanged()">
    </select>
      <label for="second_level_service_name">二级服务:</label>
      <select class="form-control" id="second_level_service_name"  >
      </select>
      <label for="load_level">负载容量比:</label>
      <select class="form-control" id="load_level"  >
          <option value="0">任意值</option>
          <option value="20">大于20%</option>
          <option value="40">大于40%</option>
          <option value="60">大于60%</option>
          <option value="80">大于80%</option>
      </select>
  </div>

  <button type="button" class="btn-small" id="btn_query" onclick="onQueryCapacityListBtnClicked()">查询</button>


</form>

</div>
<div id="result_message" class="result_msg_style">
</div>

<div id="result_table" class="table_style">

<table class="table table-hover" >
  <thead id="my_head" hidden="true">
    <td>业务</td><td>平均负载容量百分比</td><td>有效上报IP/配置IP数</td><td>操作</td>
  </thead>
  <tbody id="my_table">
  </tbody>
</table>
  </div>

<div style="display:none;width:100px;margin:0 auto;position:fixed;left:45%;top:45%;" id="div_loading">
    <img src="/imgs/progress.gif" />
</div>
</body>
</html>
