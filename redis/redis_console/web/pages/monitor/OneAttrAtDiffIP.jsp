
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
<%
    String service_name=request.getParameter("service_name");
    String dt = request.getParameter("date");
    String attribute = request.getParameter("attribute");
%>
<html>
<head>
    <title>监控属性页面</title>
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
    <link rel="stylesheet" type="text/css" href="/css/jquery.datetimepicker.css" />
    <script type="text/javascript" src="/js/jquery.datetimepicker.js"></script>
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
    <script type="text/javascript" src="/js/tools.js"></script>
  <script type="text/javascript">

      var g_svc  = "<%=service_name%>";
      var g_dt = "<%=dt%>";
      var g_attr = "<%=attribute%>";
      var data_index = 0;
      var g_id_data = {};

      function showTips(str)
      {
          $("#result_message").attr("title", "message");
          $("#result_message").empty();
          $("#result_message").append(str);

          $("#result_message").dialog();

      };

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

          showChart( i);
      }

      function showChart(pgidx)
      {
          if (g_attr == null || g_attr.length < 1 ||
          g_dt == null || g_dt.length <8 ||
          g_svc == null || g_svc.length < 1)
          {
              var str="<p>"+输入的字段非法+"</p>";
              showTips(str);
              return;

          }
          var   request={
              "handleClass":"beans.service.MonitorOneAttrAtDiffIP",
              "requestBody": {"service_name":g_svc,
                  "attribute":g_attr,
                  "date":g_dt,
                   "page":pgidx}
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              var str = "";
                              var iplist = ""; // ip集中导航
                              for (i = 0; i < data.charts.length ; ++i )
                              {
                                  var   request={
                                      "handleClass":"beans.service.DownloadMonitorChart",
                                      "requestBody": {
                                          "chart_to_download": data.charts[i].chart_file_name
                                      },
                                  };
                                  var img_url = "/JsonRPCServlet?request_string="+JSON.stringify(request);

                                  var title = "["+data.charts[i].server_ip+"] -- [Attribute: "+g_attr+"]";
                                  g_id_data[i] = data.charts[i].valuePerDay[0].values;

                                  var fmtstr = '<div align="middle">'+
                                          '<a name="anchor%d"></a>'+
                                          '<a href="javascript:showDataDialog(\'%s\',\'%s\',%d)">数值查看</a>&nbsp;&nbsp;'+
                                          '<a href="javascript:showWeekDialog(\'%s\', \'%s\', \'%s\', \'%s\', \'%s\')">周图</a>&nbsp;&nbsp;'+
                                          '<a href="#top">回顶部</a><br>'+
                                          '<image id="image%d" src=\'%s\' />'+
                                          '<hr style=" height:2px;border:none;border-top:1px dotted #185598;width: 500px" />'+
                                          '</div><br>';
                                  str +=  sprintf(fmtstr,
                                          i,
                                          title, g_dt, i,
                                          title, g_svc, g_dt, data.charts[i].server_ip, data.charts[i].attribute,
                                          i, img_url);

                                  iplist += "<a href='#anchor"+i+"'>"+data.charts[i].server_ip+"</a>,&nbsp;";


                              }
                              $("#charts").empty();
                              $("#charts").append(str);

                              $("#iplist_bar").empty();
                              $("#iplist_bar").append(iplist);

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
                  "server_ip": g_week_ip,
                  "date":g_week_date,
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

      $(document).ready(function() {
          if(g_attr != null && g_attr != "") {
              document.title = "监控属性 - 查看" + g_attr;
          }
          showChart(0);
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

      });




  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;分实例查看&nbsp;&nbsp;服务：<%=service_name%>&nbsp;属性：<%=attribute%></p>
    <a name="top"></a>
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
<div id="result_message" class="result_msg_style">
</div>

<div>
    <p id="page_bar_top" align="right" style="word-break: break-all"></p>
    <div id="iplist_bar" style="word-break: break-all"></div>
    <div id="charts">

    </div>
    <p id="page_bar" align="right" style="word-break: break-all"></p>

</div>
</body>
</html>
