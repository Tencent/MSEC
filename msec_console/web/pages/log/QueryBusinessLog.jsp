
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
    <script type="text/javascript" src="/js/tools.js"/>

    <link rel="stylesheet" type="text/css" href="/css/jquery.datetimepicker.css" />
    <script type="text/javascript" src="/js/jquery.datetimepicker.js"/>

  <style>
    #title_bar{width:auto;height:auto;   margin-top: 5px }
    #query_form{width:auto;height:auto;  padding:20px;margin-top: 5px;}
    #result_message{width:auto;height:auto;  margin-top: 5px ;}
    #result_table{width:auto;height:auto;  margin-top: 5px ;}
  </style>

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
      function onFieldListSelectedChanged()
      {
          var field_name = $("#field_list").val();
          if (field_name == null || field_name == "")
          {
              return;
          }
          var str = $("#more_condition").val();
          if (str == null)
          {
              str = field_name+"=''";
          }
          else {
              str = str.trim();
              if (str.length > 0)
              {
                  str += " and "+field_name+"=''";
              }
              else
              {
                  str += field_name+"=''";
              }

          }
          $("#more_condition").val(str);

      }

      function onQueryLogBtnClicked()
      {
          var   first_level_service_name= $("#first_level_service_name").val();
          var   second_level_service_name= $("#second_level_service_name").val();
          var   request_id= $("#request_id").val();
          var   dt_begin= $("#dt_begin").val();
          var   dt_end= $("#dt_end").val();
          var   log_ip= $("#log_ip").val();
          var   more_condition= $("#more_condition").val();

          if (dt_begin == null || dt_begin.length == 0)
          {
            dt_begin = get1hrBefore();
          }
          if (dt_end == null || dt_end.length == 0)
          {
              dt_end = getNow();
          }
          var service_name = null;
          if (first_level_service_name == null || first_level_service_name.length < 1 ||
          second_level_service_name == null || second_level_service_name.length < 1)
          {
              service_name = "";
          }
          else
          {
              service_name = first_level_service_name+"."+second_level_service_name;
          }






          var   request={
              "handleClass":"beans.service.QueryBusinessLog",
              "requestBody": {"service_name":service_name,
                    "request_id":request_id,
                  "dt_begin":dt_begin,
                  "dt_end":dt_end,
                  "log_ip":log_ip,
                  "more_condition":more_condition
                    },
          };

          /*
          var tips = JSON.stringify(request);

          var url = "/JsonRPCServlet?request_string="+tips;
          window.open(url);
          return ;
          */


          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                            window.open("/pages/log/BusinessLogResult.jsp");

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
      function queryFieldList()
      {



          var   request={
              "handleClass":"beans.service.QueryBusiLogField",
              "requestBody":   {
              }
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {

                      if (status == "success") {

                          if (data.status == 0) {

                              var i;
                              $("#field_list").empty();
                              $("#field_list").append("<option value=\"\">可用的字段</option>");
                              for (i = 0; i < data.field_list.length; ++i) {
                                  var rec = data.field_list[i];

                                  var str = "<option value ='" + rec.field_name + "'>" + rec.field_name+ "</option>\r\n";
                                  $("#field_list").append(str);


                                //  var str = '<input type="checkbox" name="result_column" value="' + rec.field_name + '" checked>'+rec.field_name+'&nbsp;&nbsp;';
                              //    $("#div_result_column").append(str);
                              }
                          }


                      }
                      else
                      {
                          showTips(status);
                          return;
                      }

                  });
      }


      $(document).ready(function() {

          query_service_list();


          $('#dt_begin').datetimepicker({format:'Ymd H:i', step:15, allowBlank:true}, $("#right"));
          $('#dt_end').datetimepicker({format:'Ymd H:i', step:15, allowBlank:true}, $("#right"));





      });

      function onMoreFieldBtnClicked()
      {
          $('#div_more_field').toggle();
          if ($("#div_more_field").is(":hidden"))
          {
              $("#btn_more_field").text('高级 ∧');
          }
          else
          {
              $("#btn_more_field").text('高级 ∨');

          }
      }

      function onLogSettingClicked()
      {
          $("#right").load("/pages/log/SetBusinessLog.jsp");
      }






  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;日志查询 <button type="button" clas="btn-small" style="float: right;font-size: 8px" onclick="onLogSettingClicked()">setting...</button>   </p>
</div>

<div id="query_form" class="form_style">
<form class="form-inline">
  <div class="form-style">
      <table border="0px">
          <tr>
              <td>
                <label for="first_level_service_name" class="label_style">一级服务:</label>
              </td>
              <td>
                <select class="form-control" id="first_level_service_name"   onchange="onFirstServiceSelectedChanged()">
                </select>
              </td>
              <td>&nbsp;&nbsp;</td>
              <td>
                  <label for="second_level_service_name" class="label_style">二级服务:</label>
              </td>
              <td>
                  <select class="form-control" id="second_level_service_name"  >
                  </select>
              </td>
          </tr>
          <tr>
              <td>
              <label for="request_id" class="label_style">请求ID:</label>
              </td>
              <td>
              <input type="text" id="request_id" class="form-control" placeholder="可选" >
              </td>
              <td>&nbsp;&nbsp;</td>
              <td>
              <label for="log_ip" class="label_style">日志发生IP:</label>
              </td>
              <td>
              <input type="text" id="log_ip" class="form-control" placeholder="可选">
              </td>
          </tr>
          <tr>
              <td>
              <label for="dt_begin" class="label_style">开始时间:</label>
              </td>
              <td>
              <input type="text" id="dt_begin" placeholder="默认1小时前" class="form-control" />
              </td>
              <td>&nbsp;&nbsp;</td>
              <td>
              <label for="dt_end" class="label_style">结束时间:</label>
              </td>
              <td>
              <input type="text" id="dt_end" placeholder="默认当前时间" class="form-control" />
              </td>
          </tr>
          <tr>
              <td>
                  <label for="more_condition"  class="label_style">进一步过滤条件:</label>
              </td>
              <td colspan="4">
                  <input type="text" id="more_condition" style="width: 80%" class="form-control" placeholder="sql语句的更多where条件，不要包含where关键字"/><br>
              </td>
          </tr>
          <tr>




          </tr>
      </table>

      <button type="button" class="btn-small" id="btn_query" onclick="onQueryLogBtnClicked()">查询</button>


  </div>
    <br>



</form>



</div>



<div id="result_message" class="result_msg_style">
</div>



<div id="result_table" class="table_style">


  </div>
</body>
</html>
