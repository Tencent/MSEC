
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

    <link rel="stylesheet" href="/js/jquery_ui/jquery-ui.theme.css">
    <link rel="stylesheet" href="/css/jquery.timepicker.css">
    <link rel="stylesheet" type="text/css" href="/css/datatables.min.css"/>
    <script type="text/javascript" src="/js/jquery.timepicker.min.js"></script>
    <script type="text/javascript" src="/js/datatables.min.js"></script>
  <style>
    #title_bar{width:auto;height:auto;   margin-top: 5px }
    #query_form{width:auto;height:auto;  padding:20px;margin-top: 5px;}
    #result_message{width:auto;height:auto;  margin-top: 5px ;}
    #result_table{width:auto;height:auto;  margin-top: 5px ;}
  </style>
    <script type="application/javascript" src="/js/tools.js"/>
    <script type="application/javascript" src="/js/ProgressTurn.js"/>
  <script type="text/javascript">
      var g_service_name ;
      var g_service_parent ;

      var data_index = 0;
      var g_id_data = {};

      function showTips(str)
      {
          $("#result_message").attr("title", "message");
          $("#result_message").empty();
          $("#result_message").append(str);

          $("#result_message").dialog();

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

          $("#right").load(url);
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

      function showWeekDialog(title, svcname, date, attr)
      {
          $('#dialog_week_chart').dialog( "option", "title", 'WeekView -- '+title );
          $('#dialog_week_chart').dialog("open");
          $('#week_chart_date').val(date);
          g_week_svc = svcname;
          g_week_attr = attr;
          g_week_date = date;
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
                              for (i = 0; i < data.charts.length ; ++i )
                              {
                                  var   request={
                                      "handleClass":"beans.service.DownloadMonitorChart",
                                      "requestBody": {
                                          "chart_to_download": data.charts[i].chart_file_name
                                      },
                                  };
                                  var img_url = "/JsonRPCServlet?request_string="+JSON.stringify(request);

                                  var title = "[Service: "+svcname+"] -- [Attribute: "+data.charts[i].attribute+"]";

                                  g_id_data[i] = data.charts[i].valuePerDay[0].values;
                                  var fmtstr = '<div align="middle">'+
                                                '<a name="anchor%d"></a>'+
                                                '<a href="javascript:diffIP(\'%s\', \'%s\', \'%s\')">按IP查看</a>&nbsp;&nbsp;'+
                                                '<a href="javascript:showDataDialog(\'%s\', \'%s\', %d)">数值查看</a>&nbsp;&nbsp;'+
                                                '<a href="javascript:showWeekDialog(\'%s\', \'%s\', \'%s\', \'%s\')">周图</a>&nbsp;&nbsp;'+
                                                '<a href="#top">回顶部</a><br>'+
                                                '<image id="image%d" src=\'%s\' />'+
                                                '<hr style=" height:2px;border:none;border-top:1px dotted #185598;width: 500px" />'+
                                                '</div><br>';
                                  str +=  sprintf(fmtstr,i,
                                          svcname,dt,data.charts[i].attribute,
                                          title, dt, i,
                                          title, svcname,dt,data.charts[i].attribute,
                                          i, img_url);
                                  attrs += "<a href='#anchor"+i+"'>"+data.charts[i].attribute+"</a>,&nbsp;";




                              }

                              if (str.length == 0)
                              {
                                  str = "<div>没有监控数据可显示。</div>";
                              }
                              $("#charts").empty();
                              $("#charts").append(str);

                              $("#attrs_bar").empty();
                              $("#attrs_bar").append(attrs);

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



      function onMonitorBtnClicked()
      {
          var   first_level_service_name= $("#first_level_service_name").val();
          var   second_level_service_name= $("#second_level_service_name").val();
          var   service_name = first_level_service_name+"."+second_level_service_name;
	  
	  g_service = second_level_service_name;
	  g_service_parent = first_level_service_name;
          var   server_ip= $("#server_ip").val();
          console.log(server_ip);
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
          $("#hidden_svcname").val(service_name);
          $("#hidden_ip").val(server_ip);
          $("#hidden_attr").val(attribute);
          $("#hidden_dt").val(date);

          showChart(service_name, server_ip, attribute, date, 0);



      }



      $(document).ready(function() {

          query_service_list();
	  
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


      function onQueryAlarmClicked()
      {
          $("#right").load("/pages/monitor/QueryAlarm.jsp");
      }
      function fillAttr(str)
      {
          if (str == "U")
          {
              $("#attribute").val("^usr.*$");
          }
          if (str == "F")
          {
              $("#attribute").val("^frm.*$");
          }
          if (str == "S")
          {
              $("#attribute").val("^sys.*$");
          }
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

      <table border="0" >
          <tr>
              <td>
                    <label for="first_level_service_name" class="label_style" >一级服务:</label>
              </td>
              <td>
                    <select class="form-control" id="first_level_service_name"   onchange="onFirstServiceSelectedChanged()">
                    </select>
                      <label for="second_level_service_name" class="label_style">二级服务:</label>
                      <select class="form-control" id="second_level_service_name"  >
                      </select>
              </td>
              <td>
                      <label for="server_ip" class="label_style">Server IP:</label>
                      <input  class="form-control"  type="text" id="server_ip" placeholder="可选">
              </td>
          </tr>
          <tr>
              <td>
                  <label for="attribute" class="label_style">属性名:</label>
              </td>
              <td>
                  <input  class="form-control" type="text" id="attribute" style="width: 320px"  placeholder="可用英文;分割多个,或者^开头的正则表达式">
              </td>
              <td>
                  &nbsp;
              <button type="button" class="btn-smaller" id="btn_user_property" style="font-size: 6px;" onclick="fillAttr('U')" >U</button>
              <button type="button" class="btn-smaller" id="btn_sys_property" style="font-size: 6px;"  onclick="fillAttr('S')" >S</button>
              <button type="button" class="btn-smaller" id="btn_frm_property" style="font-size: 6px;"  onclick="fillAttr('F')" >F</button>
              </td>
          </tr>
          <tr>
              <td>
                    <label for="attribute" class="label_style">日期:</label>
              </td>
              <td>
                    <input  class="form-control" type="text" id="date" style="width: 320px" placeholder="yyyymmdd,可选.可输入两日期来对比,用;分割" >
              </td>
              <td>

              </td>
          </tr>

      </table>
      <button type="button" class="btn-small" id="btn_query" onclick="onMonitorBtnClicked()" >查看</button>


  </div>






</form>

</div>

<div id="result_message" class="result_msg_style">
</div>

<div>
    <a name="top"></a>
    <p id="page_bar_top" align="right" style="word-break: break-all"></p>

    <div id="attrs_bar" style="word-break: break-all"></div><br>

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
