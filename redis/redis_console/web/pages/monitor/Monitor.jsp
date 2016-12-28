
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


<%@ page contentType="text/html;charset=UTF-8" language="java" %>
<html>
<%
    String service = request.getParameter("service");
    String service_parent = request.getParameter("service_parent");
    String host = request.getParameter("host");
    if(host == null)  host = "";
%>
<head>
    <title>监控页面</title>
    <link rel="stylesheet" type="text/css" href="/css/bootstrap.min.css"/>

    <link rel="stylesheet" href="/css/main.css">
    <link rel="stylesheet" href="/js/jquery_ui/jquery-ui.css">
    <link rel="stylesheet" href="/js/jquery_ui/jquery-ui.structure.css">
    <link rel="stylesheet" href="/js/jquery_ui/jquery-ui.theme.css">
    <link rel="stylesheet" href="/css/jquery.timepicker.css">
    <link rel="stylesheet" type="text/css" href="/css/datatables.min.css"/>
    <link rel="stylesheet" type="text/css" href="/css/select.dataTables.min.css">

    <script type="text/javascript" src="/js/jquery-2.2.0.min.js"></script>
    <script type="text/javascript" src="/js/jquery.form.js"></script>
    <script type="text/javascript" src="/js/jquery.cookie.js"></script>
    <script type="text/javascript" src="/js/jquery_ui/jquery-ui.min.js"></script>
    <script type="text/javascript" src="/js/tools.js"></script>
    <script type="text/javascript" src="/js/ProgressTurn.js"></script>
    <script type="text/javascript" src="/js/jquery.timepicker.min.js"></script>
    <script type="text/javascript" src="/js/datatables.min.js"></script>
    <script type="text/javascript" src="/js/dataTables.select.min.js"></script>
  <style>
    #title_bar{width:auto;height:auto;   margin-top: 5px }
    #query_form{width:auto;height:auto;  padding:20px;margin-top: 5px;}
    #result_message{width:auto;height:auto;  margin-top: 5px ;}
    #result_table{width:auto;height:auto;  margin-top: 5px ;}
  </style>
  <script type="text/javascript">

      var g_service_name = "<%=service%>";
      var g_service_parent = "<%=service_parent%>";
      var g_host = "<%=host%>";

      var data_index = 0;
      var g_id_data = {};

      function showTips(str)
      {
          $("#result_message").attr("title", "message");
          $("#result_message").empty();
          $("#result_message").append(str);

          $("#result_message").dialog();

      };

      function showPageBar(pageBar, page_num, pageIdx, seg_idx)
      {
          console.log("seg_idx:", seg_idx);
          if (page_num > 10) {
              var frm = seg_idx * 10;
              var to = frm + 10;
              if (to > page_num) { to = page_num;}
              var page_bar_str = "pages: ";
              if (frm > 0)
              {
                  var s = sprintf("<a href='javascript:showPageBar(\"%s\", %d, %d, %d)'>←</a>&nbsp;", pageBar, page_num, pageIdx, seg_idx - 1);
                  page_bar_str += s;
              }


              for (i = frm; i < to; ++i) {
                  if (pageIdx == i) {
                      page_bar_str += "<a href='javascript:goPage(" + i + ")'>[" + i + "]</a>&nbsp;&nbsp;";
                  }
                  else {
                      page_bar_str += "<a href='javascript:goPage(" + i + ")'>" + i + "</a>&nbsp;&nbsp;";
                  }
              }

              if ((to) < page_num)
              {
                  var s = sprintf("<a href='javascript:showPageBar(\"%s\", %d, %d, %d)'>→</a>&nbsp;", pageBar, page_num, pageIdx, seg_idx+1);
                  page_bar_str += s;
              }
          }
          else
          {
              var page_bar_str = "pages: ";


              for (i = 0; i < page_num; ++i) {
                  if (pageIdx == i) {
                      page_bar_str += "<a href='javascript:goPage(" + i + ")'>[" + i + "]</a>&nbsp;&nbsp;";
                  }
                  else {
                      page_bar_str += "<a href='javascript:goPage(" + i + ")'>" + i + "</a>&nbsp;&nbsp;";
                  }
              }


          }
          $("#"+pageBar).empty();
          $("#"+pageBar).append(page_bar_str);

          $("#"+pageBar+"_top").empty();
          $("#"+pageBar+"_top").append(page_bar_str);

      }
      function goPage(i)
      {

          var   service_name= $("#hidden_svcname").val();

          var   server_ip= $("#hidden_ip").val();
          var attribute = $("#hidden_attr").val();
          var  date = $("#hidden_dt").val();

          if (date == null || date.length < 1)
          {
              date = getNow_yyyyMMdd();
          }
          showChart(service_name, server_ip, attribute, date, i);
      }
      function diffIP(svcname, dt, attr)
      {
          var url = "/pages/monitor/OneAttrAtDiffIP.jsp?service_name="+encodeURIComponent(svcname);
          url +="&date="+dt;
          url+= "&attribute="+encodeURIComponent(attr);

          window.location.assign(url);
      }

      function showDataDialog(title, date, index)
      {
          data_index = index;
          $('#dialog_data').dialog( "option", "title", title );
          $('#dialog_data').dialog("open");
          $('#data_date').text(date);
          $('#data_begin').timepicker({ 'timeFormat': 'H:i' });
          $('#data_end').timepicker({ 'timeFormat': 'H:i' });
          var midnight = new Date();  //midnight
          midnight.setHours(0,0,0,0);
          var next_midnight = new Date();
          next_midnight.setHours(23,59,59,999);
          var d = new Date();
          d.setMinutes(d.getMinutes() - 30);
          if(d<midnight)    //超过限制
          {
              $('#data_begin').timepicker('setTime', midnight);
          }
          else {
              $('#data_begin').timepicker('setTime', d);
          }
          d.setMinutes(d.getMinutes() + 60);
          if(d>next_midnight)    //超过限制
          {
              $('#data_end').timepicker('setTime', next_midnight);
          }
          else {
              $('#data_end').timepicker('setTime', d);
          }
          onGetDataBtnClicked();
      }

      function pad2(n){
          return n > 9 ? "" + n: "0" + n;
      }

      function onGetDataBtnClicked() {
          var arr_data = [];
          begin_idx = $('#data_begin').timepicker('getSecondsFromMidnight')/60;
          end_idx = $('#data_end').timepicker('getSecondsFromMidnight')/60;
          if(begin_idx >= end_idx || end_idx >= g_id_data[data_index].length) {
              showTips("输入时间段有误");
              return;
          }

          var sum = 0;
          var max = 0;
          var min = 0;
          var times = 0;

          for(i = begin_idx; i <= end_idx; i++) {
              var arr = [];
              arr.push(pad2(Math.floor(i/60))+":"+ pad2(i%60));
              arr.push(g_id_data[data_index][i]);
              if(g_id_data[data_index][i] != 0) {
                  sum+=g_id_data[data_index][i];
                  max=max<g_id_data[data_index][i]?g_id_data[data_index][i]:max;
                  min=(min == 0 || min>g_id_data[data_index][i])?g_id_data[data_index][i]:min;
                  times++;
              }
              arr_data.push(arr);
          }
          $("#div_data_stats").text("Stats: Sum: "+sum+", Max: "+max+", Min: "+min +", Mean: "+Math.round(sum/times) );

          if( $.fn.dataTable.isDataTable( '#table_data' ))
          {
              $("#table_data").DataTable().clear().rows.add(arr_data).draw();
          }
          else
          {
              $("#table_data").DataTable({
                  data: arr_data,
                  columns: [
                      {title: "时间"},
                      {title: "数值"},
                  ]
              });
          }
      }

      var g_week_svc = "";
      var g_week_attr = "";
      var g_week_date = "";
      var g_week_ip = "";

      function onWeekQueryClicked(day)
      {
          if(day != 0)
          {
              d = $('#week_chart_date').datepicker("getDate");
              d.setDate(d.getDate()+day);
              $('#week_chart_date').datepicker("setDate", d);
          }
          g_week_date = $('#week_chart_date').val();
          $("#div_week_stats").empty();
          $("#div_week_chart").empty();
          var   request={
              "handleClass":"beans.service.MonitorAttrContDays",
              "requestBody": {"service_name": g_week_svc,
                  "attribute":g_week_attr,
                  "date":g_week_date,
                  "server_ip":g_week_ip,
                  "duration":7,
              },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功
                              var request={
                                  "handleClass":"beans.service.DownloadMonitorChart",
                                  "requestBody": {
                                      "chart_to_download": data.chart_file_name
                                  },
                              };
                              var str = '<image src=\'/JsonRPCServlet?request_string='+ JSON.stringify(request) + '\' />';
                              $("#div_week_chart").append(str);
                              var statsstr = "Stats: Sum: "+data.sum+", Max: "+data.max+", Min: "+data.min;
                              $("#div_week_stats").append(statsstr);
                          }
                          else if (data.status == 99) {
                              document.location.href = "/pages/users/login.jsp";
                          }
                          else {
                              //showTips(data.message);//业务处理失败的信息
                              var str = "<p>" + data.message + "</p>";
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

      function showWeekDialog(title, svcname, date, ip, attr)
      {
          $('#dialog_week_chart').dialog( "option", "title", 'WeekView -- '+title );
          $('#dialog_week_chart').dialog("open");
          $('#week_chart_date').val(date);
          g_week_svc = svcname;
          g_week_attr = attr;
          g_week_date = date;
          g_week_ip = ip;
          onWeekQueryClicked(0);
      }

      function showChart(svcname,ip,attr,dt,pgidx)
      {
          var   request={
              "handleClass":"beans.service.MonitorBySvcOrIP",
              "requestBody": {"service_name":svcname,
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

                              var str = "";
                              var attrs = "";//属性集中导航
                              var host_map = {"":"汇总监控"};
                              for (i = 0; i < data.charts.length ; ++i ) {
                                  var request = {
                                      "handleClass": "beans.service.DownloadMonitorChart",
                                      "requestBody": {
                                          "chart_to_download": data.charts[i].chart_file_name
                                      },
                                  };
                                  var img_url = "/JsonRPCServlet?request_string=" + JSON.stringify(request);

                                  var title = "[Service: " + svcname + "] -- [Attribute: " + data.charts[i].attribute + "]";

                                  g_id_data[i] = data.charts[i].valuePerDay[0].values;


                                  var week_svcname = "";
                                  if(g_host=="" || g_host.indexOf(":")>=0) {
                                      week_svcname = svcname;
                                  }

                                  var fmtstr = '<div align="middle">' +
                                          '<a name="anchor%d"></a>' +
                                          '<a href="javascript:showDataDialog(\'%s\', \'%s\', %d)">数值查看</a>&nbsp;&nbsp;' +
                                          '<a href="javascript:showWeekDialog(\'%s\', \'%s\', \'%s\', \'%s\', \'%s\')">周图</a>&nbsp;&nbsp;' +
                                          '<a href="#top">回顶部</a><br>' +
                                          '<image id="image%d" src=\'%s\' />' +
                                          '<hr style=" height:2px;border:none;border-top:1px dotted #185598;width: 500px" />' +
                                          '</div><br>';
                                  str += sprintf(fmtstr, i,
                                          title, dt, i,
                                          title, week_svcname, dt, g_host, data.charts[i].attribute,
                                          i, img_url);
                                  attrs += "<a href='#anchor"+i+"'>"+data.charts[i].attribute+"</a>,&nbsp;";
                              }

                              if(data.hosts.length > 0) {
                                  for(i = 0 ; i < data.hosts.length; ++i)
                                  {
                                      //host_map[data.hosts[i].ip] = data.hosts[i].ip;
                                      for (j = 0; j < data.hosts[i].ports.length; ++j) {
                                          var host = data.hosts[i].ip + ":" + data.hosts[i].ports[j];
                                          host_map[host] = host;
                                      }
                                  }
                              }

                              if (str.length == 0)
                              {
                                  str = "<div>没有监控数据可显示。</div>";
                              }
                              $("#charts").empty();
                              $("#charts").append(str);

                              $("#attrs_bar").empty();
                              $("#attrs_bar").append(attrs);

                              $("#server_ip").empty();
                              $.each(host_map, function(key, value) {
                                  $('#server_ip').append($('<option/>', { value : key }).text(value));
                              });
                              $("#server_ip").val(g_host);

                              $("#ip_bar").empty();
                              if(g_host.indexOf(':') > -1) {
                                  $("#ip_bar").append("<a href='Monitor.jsp?service="+$("#second_level_service_name").val()+"&service_parent="+$("#first_level_service_name").val()+"&host="+g_host.split(':')[0]+"' target='_blank'>IP基础属性查看</a>");
                              }

                              if (data.page_num > 1)
                              {
                                  var seg_idx = parseInt( data.page_idx / 10);
                                  showPageBar("page_bar", data.page_num, data.page_idx, seg_idx);

                              }
                              else
                              {
                                  $("#page_bar").empty();
                                  $("#page_bar_top").empty();
                              }

                              $("#result_message").empty();

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

                              $("#page_bar").empty();
                              $("#page_bar_top").empty();
                              $("#charts").empty();

                          }
                      }
                      else
                      {
                          //showTips(status);//http通信失败的信息
                          var str="<p>"+status+"</p>";
                          showTips(str);

                          $("#page_bar").empty();
                          $("#page_bar_top").empty();
                          $("#charts").empty();
                      }

                  });

      }



      function onMonitorBtnClicked() {
          g_host = $("#server_ip").val();
          if(g_host == "") {
              document.title = "监控 - 查看" + g_service_parent + "." + g_service_name;
          }
          else {
              document.title = "监控 - 查看" + g_host;
          }

          var first_level_service_name = $("#first_level_service_name").val();
          var second_level_service_name = $("#second_level_service_name").val();
          var service_name = first_level_service_name + "." + second_level_service_name;
          if (first_level_service_name == "")
          {
              service_name = second_level_service_name;
          }
          var server_ip= $("#server_ip").val();
          console.log(server_ip);
          var attribute = $("#attribute").val();
          var  date = $("#date").val();
          if (date == null || date.length < 1)
          {
              date = getNow_yyyyMMdd();
          }

          //保存起来，供翻页用，避免翻页的条件因为页面顶部的输入框的改动而改动
          $("#hidden_svcname").val(service_name);
          $("#hidden_ip").val(server_ip);
          $("#hidden_attr").val(attribute);
          $("#hidden_dt").val(date);
          showChart(service_name, server_ip, attribute, date, 0);
      }



      $(document).ready(function() {

          $("#first_level_service_name").val(g_service_parent);
          $("#second_level_service_name").val(g_service_name);
          $( "#dialog_data" ).dialog({
              autoOpen: false,
              height: 660,
              width: 500,
              modal: true,
              position: { my: "center", at: "left+800px top+500px ", of: window  } ,
              resizable: false,
              close: function() {
                  if( $.fn.dataTable.isDataTable( '#table_data' ))
                  {
                      $("#table_data").DataTable().destroy();
                  }
              }
          });

          $( "#week_chart_date" ).datepicker({
              dateFormat: 'yymmdd',
              changeMonth: true,
              changeYear: true
          });

          $( "#dialog_week_chart" ).dialog({
              autoOpen: false,
              height: 430,
              width: 550,
              modal: true,
              position: { my: "center", at: "left+800px top+500px ", of: window  } ,
              resizable: false,
          });
          $("#server_ip").append($('<option/>', { value : g_host }).text(g_host));
          onMonitorBtnClicked();
      });


      function onQueryAlarmClicked()
      {

          var   first_level_service_name= $("#first_level_service_name").val();
          var   second_level_service_name= $("#second_level_service_name").val();
          first_level_service_name = encodeURI(first_level_service_name);
          second_level_service_name = encodeURI(second_level_service_name);

          var url = "/pages/monitor/QueryAlarm.jsp?service=" + second_level_service_name + "&service_parent="+first_level_service_name;
        //  alert(url);
          window.location.assign(url);
      }

  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;监控
      <button type="button" clas="btn-small" style="float: right;font-size: 8px" onclick="onQueryAlarmClicked()">告警...</button>
  </p>
</div>

<div id="query_form" class="form_style">
<form class="form-inline">

  <div class="form-group">
    <label for="first_level_service_name">一级服务:</label>
    <input class="form-control" id="first_level_service_name"   readonly>
    <label for="second_level_service_name">二级服务:</label>
    <input class="form-control" id="second_level_service_name" readonly >
    <label for="server_ip">实例信息:</label>
    <select  class="form-control"  type="text" id="server_ip"></select>
      <br/>
    <label for="attribute">属性名:</label>
    <input  class="form-control" type="text" id="attribute" style="width: 320px"  placeholder="可选,可用英文;分割多个,或者^开头的正则表达式">
    <br/>
    <label for="attribute">日期:</label>
    <input  class="form-control" type="text" id="date" style="width: 320px" placeholder="yyyymmdd,可选.可输入两日期来对比,用;分割" >
    <br/>
    <button type="button" class="btn-small" id="btn_query" onclick="onMonitorBtnClicked()">查看</button>
  </div>
</form>

</div>

<div id="result_message" class="result_msg_style">
</div>

<div>
    <a name="top"></a>
    <p id="page_bar_top" align="right" style="word-break: break-all"></p>
    <div id="attrs_bar" style="word-break: break-all"></div><br>
    <div id="ip_bar" style="word-break: break-all"></div><br>
    <div id="charts"></div>
    <input type="hidden" id="hidden_svcname" />
    <input type="hidden" id="hidden_ip" />
    <input type="hidden" id="hidden_attr"/>
    <input type="hidden" id="hidden_dt"/>
    <p id="page_bar" align="right" style="word-break: break-all"></p>
</div>
<div id="dialog_data">
    <span class="ui-helper-hidden-accessible"><input type="text"/></span>
    <div>
        <label for="data_date">日期:</label>
        <span id="data_date"></span>
        <label for="data_begin">起始:</label>
        <input type="text" id="data_begin" class="time">
        <label for="data_end">结束:</label>
        <input type="text" id="data_end" class="time">
        <button type="button" class="btn-small" id="btn_data_query" onclick="onGetDataBtnClicked()">查看</button>
    </div>
    <div id="div_data_stats" class="title_bar_style">
    </div>
    <div>
        <table id="table_data" class="table table-hover">
        </table>
    </div>
</div>
<div id ="dialog_week_chart">
    <span class="ui-helper-hidden-accessible"><input type="text"/></span>
    <div>
        <label for="week_chart_date">日期:</label>
        <input type="text" id="week_chart_date">
        <button type="button" class="btn-icon icon-weekquery" id="icon_week_query" onclick="onWeekQueryClicked(0)" title="查询"></button>&nbsp;
        <button type="button" class="btn-icon icon-beforeday" id="icon_before_day" onclick="onWeekQueryClicked(-1)" title="前一天"></button>
        <button type="button" class="btn-icon icon-afterday" id="icon_after_day" onclick="onWeekQueryClicked(1)" title="后一天"></button>
        <button type="button" class="btn-icon icon-beforeweek" id="icon_before_week" onclick="onWeekQueryClicked(-7)" title="前一周"></button>
        <button type="button" class="btn-icon icon-afterweek" id="icon_after_week" onclick="onWeekQueryClicked(7)" title="后一周"></button>

    </div>
    <div id="div_week_stats" class="title_bar_style">
    </div>
    <div id="div_week_chart">
    </div>
</div>
<div style="display:none;width:100px;margin:0 auto;position:fixed;left:45%;top:45%;" id="div_loading">
    <img src="/imgs/progress.gif" />
</div>
</body>
</html>
