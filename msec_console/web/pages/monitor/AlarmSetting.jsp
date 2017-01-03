
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
    <script type="application/javascript" src="/js/tools.js"/>
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


                              var str1 = "";
                              for (prop in g_service_relation )
                              {
                                  str1 += "<option value=\""+prop+"\">"+prop+"</option>";
                              }
                              $("#first_level_service_name").empty();
                              $("#first_level_service_name").append(str1);

                              $("#new_first_level_service_name").empty();
                              $("#new_first_level_service_name").append(str1);

                              onFirstServiceSelectedChanged();
                              onFirstServiceSelectedChanged2();




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

          if ( first_name != null && first_name != "" )
          {
              if (first_name in g_service_relation)
              {
                  var arr = g_service_relation[first_name];
                  var str = "";
                  //  str += "<option value=\"\">所有</option>";
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

      function onFirstServiceSelectedChanged2()
      {
          var first_name = $("#new_first_level_service_name").val();

          if ( first_name != null && first_name != ""  )
          {
              if (first_name in g_service_relation)
              {
                  var arr = g_service_relation[first_name];
                  var str = "";
                  //  str += "<option value=\"\">所有</option>";
                  var i;
                  for (i = 0; i < arr.length; ++i)
                  {
                      var prop = arr[i];
                      str += "<option value=\""+prop+"\">"+prop+"</option>";
                  }
                  $("#new_second_level_service_name").empty();
                  $("#new_second_level_service_name").append(str);
              }
          }

      }

      function delAlarmSetting(svc, attr,  alarmType, rowid)
      {
          var   request={
              "handleClass":"beans.service.DeleteAlarmSetting",
              "requestBody": {"service_name":svc,
                 "attr_name":attr,
                  "alarm_type":alarmType
              },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              $("#"+rowid).hide();


                          }
                          else if (data.status == 99)
                          {
                              document.location.href = "/pages/users/login.jsp";
                          }
                          else
                          {
                              //showTips(data.message);//业务处理失败的信息
                              var str=data.message;
                              showTips(str);
                          }
                      }
                      else
                      {
                          //showTips(status);//http通信失败的信息
                          var str=status;
                          showTips(str);
                      }

                  });


      }

      function addAlarmSetting()
      {
          var   first_level_service_name= $("#new_first_level_service_name").val();
          var   second_level_service_name= $("#new_second_level_service_name").val();
          var   service_name = first_level_service_name+"."+second_level_service_name;
          if (service_name == ".")
          {
              showTips("业务名不能为空.");
              return;
          }
          var alarm_type = parseInt( $("#new_alarm_type").val());
          var threshold = $("#new_threshold").val();
          var attr = $("#new_attribute").val();
          if (attr == "")
          {
              showTips("属性名不能为空.");
              return;
          }

          var   request={
              "handleClass":"beans.service.AddAlarmSetting",
              "requestBody": {"service_name":service_name,
                  "attr_name":attr,
                  "alarm_type":alarm_type,
                  "threshold":threshold
              }
          };
          console.log(JSON.stringify(request));

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              showTips("新增成功");

                          }
                          else if (data.status == 99)
                          {
                              document.location.href = "/pages/users/login.jsp";
                          }
                          else
                          {
                              //showTips(data.message);//业务处理失败的信息
                              var str=data.message;
                              showTips(str);
                          }
                      }
                      else
                      {
                          //showTips(status);//http通信失败的信息
                          var str=status;
                          showTips(str);
                      }

                  });


      }

      function getAlarmTypeInfo(type)
      {
          if (type == 1) return "超过最大值";
          if (type == 2) return "低于最小值";
          if (type == 3) return "波动过大";
          if (type == 4) return "波动百分比过大";
      }
      function queryAlarmSetting()
      {
          var   first_level_service_name= $("#first_level_service_name").val();
          var   second_level_service_name= $("#second_level_service_name").val();
          var   service_name = first_level_service_name+"."+second_level_service_name;
          if (service_name == ".")
          {
              service_name = "";
          }




          var   request={
              "handleClass":"beans.service.QueryAlarmSetting",
              "requestBody": {"service_name":service_name},
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              var str = "";

                              if (data.alarm_list.length == 0)
                              {
                                  str="没有告警设置。";

                                  showTips(str);
                                  $("#my_table").empty();
                                  return;
                              }

                              $.each(data.alarm_list, function (i, rec) {
                                  str += "<tr id='row"+i+"'>";
                                  str += "<td>" + rec.service_name+ "</td>";
                                  str += "<td>" +rec.attribute_name  + "</td>";

                                  str += "<td>" + getAlarmTypeInfo(rec.alarm_type)+"</td>";
                                  str += "<td>" +rec.threshold+"</td>";
                                  str += "<td>";

                                  str += "<a href='javascript:delAlarmSetting(\""+rec.service_name+"\", \""+rec.attribute_name+"\","+rec.alarm_type+",\"row"+i+"\")'>删除</a>";


                                  str += "</td>";
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
                              var str=data.message;
                              showTips(str);
                          }
                      }
                      else
                      {
                          //showTips(status);//http通信失败的信息
                          var str=status;
                          showTips(str);
                      }

                  });

      }

      function hideOtherParts(me)
      {
          var div_list={list:["#div_query_setting","#div_add_setting"]};
          var btn_list={list:["#btn_toggle_query", "#btn_toggle_add"]};
          $.each(div_list.list, function(i, rec){
              if (rec != me)
              {
                  $(rec).hide();
                  $(btn_list.list[i]).text("+");
              }
          })

      }

      function addingToggle()
      {
          $("#div_add_setting").toggle();
          if ($("#div_add_setting").is(":hidden"))
          {
              $("#btn_toggle_add").text('+');
          }
          else
          {
              $("#btn_toggle_add").text('-');
              hideOtherParts("#div_add_setting");
          }
      }
      function queryToggle()
      {
          $("#div_query_setting").toggle();
          if ($("#div_query_setting").is(":hidden"))
          {
              $("#btn_toggle_query").text('+');
          }
          else
          {
              $("#btn_toggle_query").text('-');
              hideOtherParts("#div_query_setting");
          }
      }






      $(document).ready(function() {

          query_service_list();

          addingToggle();
          queryToggle();

      });


      function onQueryAlarmClicked()
      {
          $("#right").load("/pages/monitor/QueryAlarm.jsp");
      }






  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;告警设置
      <button type="button" clas="btn-small" style="float: right;font-size: 8px" onclick="onQueryAlarmClicked()">告警...</button>
  </p>
</div>

<div  class="form_style">
    <button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_query" onclick="queryToggle()">-</button>&nbsp;查询告警设置<br><br>
    <div id="div_query_setting">
<form class="form-inline">

  <div class="form-group">

    <label for="first_level_service_name">一级服务:</label>
    <select class="form-control" id="first_level_service_name"   onchange="onFirstServiceSelectedChanged()">
    </select>
      <label for="second_level_service_name">二级服务:</label>
      <select class="form-control" id="second_level_service_name"  >
      </select>

      &nbsp;&nbsp;&nbsp;&nbsp;<button type="button" class="btn-small" id="btn_query" onclick="queryAlarmSetting()">查看</button>

  </div>

</form>
        <div id="result_table" class="table_style">

            <table class="table table-hover" >
                <thead>
                <td>业务</td><td>属性</td><td>类型</td><td>阈值</td><td>操作</td>
                </thead>
                <tbody id="my_table">
                    <tr>
                        <td></td>
                    </tr>
                </tbody>
            </table>
        </div>
        </div>

</div>

<div id="result_message" class="result_msg_style">
</div>




<div  class="form_style">
    <button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_add" onclick="addingToggle()">-</button>&nbsp;新增告警设置<br><br>
    <div id="div_add_setting">
    <form class="form-inline">

        <div class="form-group">

            <label for="new_first_level_service_name">一级服务:</label>
            <select class="form-control" id="new_first_level_service_name"   onchange="onFirstServiceSelectedChanged2()">
            </select>
            <label for="new_second_level_service_name">二级服务:</label>
            <select class="form-control" id="new_second_level_service_name"  >
            </select>
            <label for="new_attribute">属性:</label>
            <input  class="form-control" type="text" id="new_attribute"  >
            <br><br>
            <label for="new_alarm_type">类型:</label>
            <select class="form-control" id="new_alarm_type"   >
                <option value="1">超过最大值</option>
                <option value="2">低于最小值</option>
                <option value="3">波动绝对值过大</option>
                <option value="4">波动百分比过大</option>
            </select>
            <label for="new_threshold">阈值:</label>
            <input  class="form-control" type="text" id="new_threshold"  >

            &nbsp;&nbsp;&nbsp;&nbsp;<button type="button" class="btn-small" id="btn_add" onclick="addAlarmSetting()">新增</button>

        </div>

    </form>
        </div>

</div>


<div style="display:none;width:100px;margin:0 auto;position:fixed;left:45%;top:45%;" id="div_loading">
    <img src="/imgs/progress.gif" />
</div>
</body>
</html>
