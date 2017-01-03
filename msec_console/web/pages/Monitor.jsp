
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
  <script type="text/javascript">
      function showTips(str)
      {
          $("#result_message").empty();
          $("#result_message").append(str);
          //本来是不弹对话框窗口的，担心浏览器关闭，但这里页面比较长，不弹担心用户看不到信息
          alert(str);
      };
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
                            //str += "<option value=\"\">所有</option>";
                            for (prop in g_service_relation )
                            {
                                str += "<option value=\""+prop+"\">"+prop+"</option>";
                            }
                            $("#first_level_service_name").empty();
                            $("#first_level_service_name").append(str);

                            onFirstServiceSelectedChanged();



                        }
                        else if (data.status == 99)
                        {
                            document.location.href = "/pages/users/login.jsp";
                        }
                        else
                        {
                            //alert(data.message);//业务处理失败的信息
                            var str="<p>"+data.message+"</p>";
                            showTips(str);
                        }
                    }
                    else
                    {
                        //alert(status);//http通信失败的信息
                        var str="<p>"+status+"</p>";
                        showTips(str);
                    }

                });

    }
      function onFirstServiceSelectedChanged()
      {
          var first_name = $("#first_level_service_name").val();

          if ( first_name != null && first_name != ""  )
          {
              if (first_name in g_service_relation)
              {
                  var arr = g_service_relation[first_name];
                  var str = "";
                 // str += "<option value=\"\">所有</option>";
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
      function goPage(i)
      {

          var   first_level_service_name= $("#hidden_flsn").val();
          var   second_level_service_name= $("#hidden_slsn").val();
          var   server_ip= $("#hidden_ip").val();
          var attribute = $("#hidden_attr").val();
          var  date = $("#hidden_dt").val();

          if (date == null || date.length < 1)
          {
              date = getNow_yyyyMMdd();
          }
          showChart(first_level_service_name,second_level_service_name, server_ip, attribute, date, i);
      }

      function showChart(flsn,slsn,ip,attr,dt,pgidx)
      {
          var   request={
              "handleClass":"beans.service.MonitorBySvcOrIP",
              "requestBody": {"first_level_service_name":flsn,
                  "second_level_service_name":slsn,
                  "server_ip":ip,
                  "attribute":attr,
                  "date":dt,
                "page":pgidx},
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              //data.picFileNames;
                              for (i = 0; i < data.picFileNames.length ; ++i )
                              {
                                  var   request={
                                      "handleClass":"beans.service.DownloadMonitorChart",
                                      "requestBody": {
                                          "chart_to_download": data.picFileNames[i],
                                      },
                                  };
                                  var url = "/JsonRPCServlet?request_string="+JSON.stringify(request);
                                  var img = "#chart"+i;
                                  $(img).attr("src", url);
                                 // $(img).attr("height", 350);
                                 // $(img).attr("width", 490);
                                  $(img).attr("alt", "loading...");
                                  $(img).show();

                              }
                              //不足10个图，就隐藏后面的几张，避免之前的导致干扰
                              for (;i<10;++i)
                              {
                                  var img = "#chart"+i;
                                  $(img).attr("src", "");
                                  $(img).hide();
                              }

                              if (data.page_num > 1)
                              {
                                  var page_bar_str = "pages: ";
                                  for (i = 0; i < data.page_num; ++i) {
                                      if (data.page_idx == i)
                                      {
                                          page_bar_str += "<a href='javascript:goPage(" + i + ")'>[" + i + "]</a>&nbsp;&nbsp;";
                                      }
                                      else {
                                          page_bar_str += "<a href='javascript:goPage(" + i + ")'>" + i + "</a>&nbsp;&nbsp;";
                                      }

                                  }
                                  $("#page_bar").empty();
                                  $("#page_bar").append(page_bar_str);

                                  $("#page_bar_top").empty();
                                  $("#page_bar_top").append(page_bar_str);
                              }

                              $("#result_message").empty();




                          }
                          else if (data.status == 99)
                          {
                              document.location.href = "/pages/users/login.jsp";
                          }
                          else
                          {
                              //alert(data.message);//业务处理失败的信息
                              var str="<p>"+data.message+"</p>";
                              showTips(str);
                          }
                      }
                      else
                      {
                          //alert(status);//http通信失败的信息
                          var str="<p>"+status+"</p>";
                          showTips(str);
                      }

                  });

      }

      function onMonitorBtnClicked()
      {
          var   first_level_service_name= $("#first_level_service_name").val();
          var   second_level_service_name= $("#second_level_service_name").val();
          var   server_ip= $("#server_ip").val();
          var attribute = $("#attribute").val();
          var  date = $("#date").val();
          if (date == null || date.length < 1)
          {
              date = getNow_yyyyMMdd();
          }
          if (first_level_service_name == null || first_level_service_name.length < 1 ||
          second_level_service_name == null || second_level_service_name.length < 1)
          {
              confirm("服务名不能为空");
              return;
          }
          //保存起来，供翻页用，避免翻页的条件因为页面顶部的输入框的改动而改动
          $("#hidden_flsn").val(first_level_service_name);
          $("#hidden_slsn").val(second_level_service_name);
          $("#hidden_ip").val(server_ip);
          $("#hidden_attr").val(attribute);
          $("#hidden_dt").val(date);

          showChart(first_level_service_name,second_level_service_name, server_ip, attribute, date, 0);



      }



      $(document).ready(function() {

          query_service_list();

      });




  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;监控</p>
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

      <label for="service_ip">Server IP:</label>
      <input  class="form-control"  type="text" id="service_ip" placeholder="可选">

      <br><br>
      <label for="attribute">属性名:</label>
      <input  class="form-control" type="text" id="attribute" placeholder="可多个，英文分号分割">
      <label for="attribute">日期:</label>
      <input  class="form-control" type="text" id="date" placeholder="yyyymmdd，默认今天" >
      &nbsp;&nbsp;&nbsp;&nbsp;<button type="button" class="btn-small" id="btn_query" onclick="onMonitorBtnClicked()">查看</button>
  </div>






</form>

</div>
<div id="result_message" class="result_msg_style">
</div>

<div>
    <p id="page_bar_top" align="right"></p>
    <p align="middle"><image id="chart0" src="" /></p><br>

    <p align="middle"><image id="chart1" src="" /></p><br>
    <p align="middle"><image id="chart2" src="" /></p><br>
    <p align="middle"><image id="chart3" src="" /></p><br>
    <p align="middle"><image id="chart4" src="" /></p><br>
    <p align="middle"><image id="chart5" src="" /></p><br>
    <p align="middle"><image id="chart6" src="" /></p><br>
    <p align="middle"><image id="chart7" src="" /></p><br>
    <p align="middle"><image id="chart8" src="" /></p><br>
    <p align="middle"><image id="chart9" src="" /></p><br>



    <input type="hidden" id="hidden_flsn" />
    <input type="hidden" id="hidden_slsn" />
    <input type="hidden" id="hidden_ip" />
    <input type="hidden" id="hidden_attr"/>
    <input type="hidden" id="hidden_dt"/>
    <p id="page_bar" align="right"></p>

</div>
</body>
</html>
