
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
%>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<title></title>

  <style>
	#title_bar{width:auto;height:auto;   }

	#result_message{width:auto;height:auto; }

  </style>
	<script type="application/javascript" src="/js/ProgressTurn.js"></script>
	<link rel="stylesheet" type="text/css" href="/css/datatables.min.css"/>
	<link rel="stylesheet" type="text/css" href="/css/select.dataTables.min.css">
	<script type="text/javascript" src="/js/datatables.min.js"></script>
	<script type="text/javascript" src="/js/dataTables.select.min.js"></script>
  <script type="text/javascript">
	  var g_service_name = "<%=service%>";
	  var g_service_parent = "<%=service_parent%>";
	  var refresh_timeout = 2000;

	  var install_pre;
	  var remove_pre;
	  var plan_id;
	  var warning = "";
	  var refresh_id ;
	  var service_ips;
	  var added_ips;
	  var remove_ips;
	  var data_dir;

	  $(function() {
		  $( "#tabs" ).tabs().css({'min-height': '700px'});
		  $( "#cluster_plan_tabs").tabs();
	  });

	  dialog = $( "#plan" ).dialog({
		  autoOpen: false,
		  height: 640,
		  width: 700,
		  modal: true,
		  close: function() {
			  if( $.fn.dataTable.isDataTable( '#table_plan' ))
			  {
				  $("#table_plan").DataTable().destroy();
			  }
			  clearTimeout(refresh_id);
		  }
	  });

	  function clearInput()
	  {
		  $("#new_ip").val('');
		  $("#remove_ip").val('');
		  $("#plan_status").hide();
		  $("#div_new_ip_table").hide();
		  $("#div_remove_ip_table").hide();

		  isInstall = true;
		  isRemoveInstall = true;
	  }

	  function formatBytes(bytes,decimals) {
		  if(bytes == 0) return '0 Bytes';
		  var k = 1000,
			  dm = decimals + 1 || 3,
			  sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB'],
			  i = Math.floor(Math.log(bytes) / Math.log(k));
		  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
	  }

	  function getESClusterDetail()
	  {
		  if($("#table_ip_port_list").DataTable().rows().data().length == 0) {
			  console.log("getclusterdetail empty");
			  return;
		  }
		  $("#table_ip_port_list").DataTable().rows().every( function ( rowIdx, tableLoop, rowLoop ) {
			  var d = this.data();
			  d[2] = "";
			  d[3] = "";
			  d[4] = "";
			  this.invalidate();
		  } );
		  $("#recover_old_host").empty();
		  $("#div_recover_message").show();
		  $("#div_recover_process").hide();

		  $("#health_message").css('display','none');
		  $("#health_message").val("");

		  var   request={
			  "handleClass":"beans.service.QueryESClusterDetail",
			  "requestBody": {
				  "first_level_service_name":g_service_parent,
				  "second_level_service_name":g_service_name,
			  },
		  };
		  $.post("/JsonRPCServlet",
				  {request_string:JSON.stringify(request)},
				  function(data, status) {
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  if(Object.keys(data.info_map).length <=0) {
								  showTips("get status error.");
							  }
							  else
							  {
								  if(data.health_status != "init") {
									  $("#health_message").text("集群健康值：" + data.health_status + "(" + data.active_shards + " of " + data.total_shards + ")");
									  $("#health_message").css({"display": "inline", "background": data.health_status});
								  }

								  $("#table_ip_port_list").DataTable().rows().every( function ( rowIdx, tableLoop, rowLoop ) {
									  var d = this.data();
									  info = data.info_map[d[1].split(":")[0]];
									  if(info == null || !info.ok) {
										  d[2] = "-";
										  d[3] = "-";
										  d[4] = "ERROR";
										  this.nodes().to$().find('td:eq(4)').css("color", "red");
										  this.nodes().to$().find('td:eq(4)').css("fontWeight", "bold");
									  }
									  else {
										  d[2] = NumWithCommas(info.doc_count) + "(" + formatBytes(info.doc_disk_size) + ")";
										  d[3] = formatBytes(info.avail_disk_size);
										  d[4] = "OK";
										  this.nodes().to$().find('td:eq(4)').css("color", "green");
										  this.nodes().to$().find('td:eq(4)').css("fontWeight", "normal");
									  }

									  this.invalidate();
								  } );
								  $("#table_ip_port_list").DataTable().draw();

							  }
						  }
						  else	if (data.status == 99)
						  {
							  document.location.href = "/pages/users/login.jsp";
						  }
						  else//refresh service...
						  {
							  getServiceDetail();
						  }
					  }
				  });
	  }
	  function ESCmd(host, cmd)
	  {
		  var   request={
			  "handleClass":"beans.service.ESCmd",
			  "requestBody": {
				  "first_level_service_name":g_service_parent,
				  "second_level_service_name":g_service_name,
				  "host": host,
				  "command":cmd}
		  };

		  $.post("/JsonRPCServlet",
				  {request_string:JSON.stringify(request)},
				  function(data, status) {
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  if(data.result!="")
							  	showTips(data.result);
							  getServiceDetail();
						  }
						  else if (data.status == 99)
						  {
							  document.location.href = "/pages/users/login.jsp";
						  }
						  else
						  {
							  showTips(data.message);
							  return;
						  }
					  }
					  else
					  {
						  showTips(status);
						  return;
					  }
				  }
		  );
	  }

	  function detail_op_format(d)
	  {
		  var out = '<table cellpadding="5" cellspacing="0" border="0" style="padding-left:50px;"><tr>';
		  out += '<td><button type="button" class="btn-small" style="float: right;font-size: 8px" onclick=\'ESCmd("'+d[1].split(" ")[0]+'","restart")\'>实例重启</button></td>';
		  out += '</tr></table>';
		  return out;
	  }

	  function NumWithCommas(x) {
		  return x.toString().replace(/\B(?=(\d{3})+(?!\d))/g, ",");
	  }

	  function getServiceDetail()
	  {
		  clearInput();
		  $( "#tabs" ).tabs( "option", "active", 0 );

		  service_ips = [];
		  var   request={
			  "handleClass":"beans.service.QuerySecondLevelServiceDetail",
			  "requestBody": {
				  "first_level_service_name":g_service_parent,
				  "second_level_service_name":g_service_name},
		  };
		  $.post("/JsonRPCServlet",
				  {request_string:JSON.stringify(request)},
				  function(data, status){
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  var arr_ips = [];
							  plan_id = data.cluster_info.plan_id;

							  var first_level_service_name = encodeURI(g_service_parent);
							  var second_level_service_name = encodeURI(g_service_name);

							  $.each(data.servers, function (i, rec) {
								  var arr_ip = [];
								  var mon_url = "/pages/monitor/Monitor.jsp?service=" + second_level_service_name + "&service_parent="+first_level_service_name+"&host="+rec.ip;

								  var head_url = "http://"+window.location.hostname+":8081/?base_uri=http://"+window.location.hostname+":8081/"+rec.ip +"/";
								  arr_ip.push("",
										  rec.ip + ":" + rec.port+" <img src=\"imgs/monitor.png\" alt=\"监控\" style=\"width:20px; height:auto;\" onclick=\"window.open('"+mon_url+"')\">"
										  +" <img src=\"imgs/head.png\" alt=\"Head\" style=\"width:20px; height:auto;\" onclick=\"window.open('"+head_url+"')\">", "", "", "");
								  arr_ips.push(arr_ip);
								  service_ips.push(rec.ip);
							  } );

							  if( $.fn.dataTable.isDataTable( '#table_ip_port_list' ))
							  {
								  $("#table_ip_port_list").DataTable().clear().rows.add(arr_ips).draw();
							  }
							  else
							  {
								  $("#table_ip_port_list").DataTable({
									  data: arr_ips,
									  columns: [
										  {title: ""},
										  {
											  "className": 'ip-port',
											  "title": "IP:Port",
										  },
										  {title: "文档数（所用空间）"},
										  {title: "剩余空间"},
										  {title: "当前状态"},
									  ],
									  columnDefs: [ {
										  "className":      'details-control',
										  orderable: false,
										  "defaultContent": '',
										  targets:   0
									  } ],
									  select: {
										  style:    'os',
										  selector: 'td:first-child'
									  },
									  order: [[ 1, 'asc' ]]
								  });

								  // Add event listener for opening and closing details
								  $('#table_ip_port_list tbody').on('click', 'td.details-control', function () {
									  var tr = $(this).closest('tr');
									  var row = $("#table_ip_port_list").DataTable().row( tr );

									  if ( row.child.isShown() ) {
										  // This row is already open - close it
										  row.child.hide();
										  tr.removeClass('shown');
									  }
									  else {
										  // Open this row
										  row.child( detail_op_format(row.data()) ).show();
										  tr.addClass('shown');
									  }
								  } );
							  }

							  getESClusterDetail();

							  if(plan_id == "") {
								  $("#plan_status").hide();
							  }
							  else {
								  $("#plan_status").show();
							  }

						  }
						  else if (data.status == 99)
						  {
							  document.location.href = "/pages/users/login.jsp";
						  }
						  else
						  {
							  showTips(data.message);
							  return;
						  }
					  }
					  else
					  {
						  showTips(status);
						  return;
					  }

				  });

	  };

	  function onGetPlanListBtnClicked()
	  {
		  var begin_date = $("#dt_begin").val();
		  var end_date = $("#dt_end").val();

		  var request = {
			  "handleClass": "beans.service.GetPlanList",
			  "requestBody": {
				  "first_level_service_name": g_service_parent,
				  "second_level_service_name": g_service_name,
				  "begin_date": begin_date,
				  "end_date": end_date,
			  }
		  };

		  $.post("/JsonRPCServlet",
				  {request_string: JSON.stringify(request)},
				  function (data, status) {
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  var arr_plans = [];
							  $.each(data.plans, function (i, rec) {
								  var arr_plan = [];
								  arr_plan.push(rec.plan_id, rec.operation, rec.status);
								  arr_plans.push(arr_plan);
							  })

							  if( $.fn.dataTable.isDataTable( '#table_plan_list' ))
							  {
								  $("#table_plan_list").DataTable().clear().order(0,'desc').rows.add(arr_plans).draw();
							  }
							  else
							  {
								  $("#table_plan_list").DataTable({
									  data: arr_plans,
									  columns: [
										  {title: "任务ID"},
										  {title: "操作"},
										  {title: "当前状态"},
										  {title: "详情"},
									  ],
									  "columnDefs": [ {
										  "targets": -1,
										  "data": null,
										  "defaultContent": "<button>查看详情</button>"
									  } ],
									  "order": [[ 0, "desc" ]]
								  });
							  }

							  $('#table_plan_list tbody').on( 'click', 'button', function () {
								  var data = $("#table_plan_list").DataTable().row( $(this).parents('tr') ).data();
								  onGetPlanDetail( data[0] , false);
							  } );
						  }
						  else if (data.status == 99) {
							  document.location.href = "/pages/users/login.jsp";
						  }
						  else {
							  showTips(data.message);
						  }
					  }
					  else {
						  showTips(status);
					  }
				  });
	  }

	  function onGetPlanDetail(id, refresh)
	  {
		  $( "#plan" ).dialog("open");

		  if(refresh) {
			  $("#plan").dialog('option', 'title', '任务进度（定时刷新）');
		  }
		  else {
			  $("#plan").dialog('option', 'title', '任务进度');
		  }
		  var request = {
			  "handleClass": "beans.service.GetPlanDetail",
			  "requestBody": {
				  "first_level_service_name": g_service_parent,
				  "second_level_service_name": g_service_name,
				  "plan_id": id,
			  }
		  };
		  $.post("/JsonRPCServlet",
				  {request_string: JSON.stringify(request)},
				  function (data, status) {
					  if (status == "success") {//http通信返回200
						  var str = "";
						  var arr_plans = [];
						  if (data.status == 0) {//业务处理成功
							  $.each(data.servers, function (i, rec) {
								  var arr_plan = [];
								  arr_plan.push(rec.ip+":"+rec.port, rec.status);
								  arr_plans.push(arr_plan);
							  })
							  if( $.fn.dataTable.isDataTable( '#table_plan' ))
							  {
								  $("#table_plan").DataTable().clear().rows.add(arr_plans).draw();
							  }
							  else
							  {
								  $("#table_plan").DataTable({
									  data: arr_plans,
									  columns: [
										  {title: "IP:Port"},
										  {title: "任务进度"},
									  ]
								  });
							  }

							  if(data.finished) {
								  if(refresh) {
									  getServiceDetail();
								  }
							  }
							  else {
								  if(refresh) {
									  refresh_id = setTimeout(onGetPlanDetail, refresh_timeout, id, refresh);
								  }
							  }
						  }
						  else if (data.status == 99) {
							  document.location.href = "/pages/users/login.jsp";
						  }
						  else {
							  showTips(data.message);
						  }
					  }
					  else {
						  showTips(status);
					  }
				  });
	  }

	  function onNewIpAddBtnClicked()
	  {
		  install_pre = "init";
		  var new_ip= $("#new_ip").val();
		  data_dir=$("#data_dir").val();
		  if (!/^[0-9;\.]+$/.test(new_ip))
		  {
			  showTips("ip输入错误");
			  return;
		  }
		  if(data_dir.indexOf("/") != 0) {
			  showTips("日志存储路径需为绝对路径");
			  return;
		  }
		  //TODO: Add number test...
		  var   request={
			  "handleClass":"beans.service.AddSecondLevelServiceIPInfo",
			  "requestBody": {"ip": new_ip,
				  "first_level_service_name":g_service_parent,
				  "second_level_service_name":g_service_name,
				  "data_dir": data_dir,
			  }
		  };

		  $.post("/JsonRPCServlet",
				  {request_string:JSON.stringify(request)},
				  function(data, status) {
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  if(data.message == "success" && data.added_ips.length > 0) {
								  install_pre = "ok";
								  added_ips = [];
							  }
							  $("#div_new_ip_table").show();
							  $("#ip_table_body").empty();
							  $.each(data.added_ips, function(i, rec){
								  var str = "<tr><td>"+rec.ip+"</td><td>";
								  if(rec.status_message == "ok") {
									  str += "CPU：" + rec.cpu_cores + "核，" + rec.cpu_mhz +"Mhz，当前负载"+ rec.cpu_load +"%，可用内存：" + rec.memory_avail + "MB<br/>" +
											  "日志存储路径可用空间："+ rec.data_dir_free_space+"</td>";
									  added_ips.push(rec.ip);
								  }
								  else {
									  install_pre = "error";
									  if(rec.status_message == "shell") {
										  str += "<strong style=\"color:red\">无法连接，请检查IP及是否安装了Remote Shell</strong>";
									  }
								  }
								  str += "</tr>";
								  $("#ip_table_body").append(str);
							  })
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
	  };

	  function onNewIpTableAddBtnClicked() {
		  refresh_id = 0;
		  if (install_pre != "ok") {
			  showTips("请重新确认扩容方案");
			  return;
		  }
		  install_pre = "init";

		  var request = {
			  "handleClass": "beans.service.InstallPlan",
			  "requestBody": {
				  "first_level_service_name": g_service_parent,
				  "second_level_service_name": g_service_name,
				  "added_ips": added_ips,
				  "data_dir": data_dir,
			  }
		  };

		  $.post("/JsonRPCServlet",
				  {request_string: JSON.stringify(request)},
				  function (data, status) {
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  showTips("任务已成功创建，请前往任务列表查看任务进度。");
							  getServiceDetail();
						  }
						  else if (data.status == 99) {
							  document.location.href = "/pages/users/login.jsp";
						  }
						  else {
							  showTips(data.message);
						  }
					  }
					  else {
						  showTips(status);
					  }
				  });

	  }

	  function containsAll(s, pool){
		  for(var i = 0 , len = s.length; i < len; i++){
			  if($.inArray(s[i], pool) == -1) return false;
		  }
		  return true;
	  }

	  function onRemoveIpBtnClicked() {
		  remove_pre = "init";
		  var remove_ip = $("#remove_ip").val();
		  if (!/^[0-9;\.]+$/.test(remove_ip))
		  {
			  showTips("ip输入错误");
			  return;
		  }
		  if (remove_ip.charAt(remove_ip.length - 1) == ';') {
			  remove_ip = remove_ip.substr(0, remove_ip.length - 1);
		  }
		  remove_ips = remove_ip.split(";");

		  if(!containsAll(remove_ips, service_ips))
		  {
			  showTips("输入的ip列表中存在非法ip");
			  return;
		  }
		  $("#div_remove_ip_table").show();
		  var arrLength = remove_ips.length;
		  $("#remove_ip_table_body").empty();
		  var str ="";
	      for (var i = 0; i < arrLength; i++) {
			  str += "<tr><td>" + remove_ips[i] + "</td></tr>\r\n";
		  }
		  remove_pre = "ok";
		  $("#remove_ip_table_body").append(str);
	  }

	  function onRemoveIpTableBtnClicked() {
		  refresh_id = 0;
		  if (remove_pre != "ok") {
			  showTips("请重新确认缩容方案");
			  return;
		  }
		  remove_pre = "init";

		  var request = {
			  "handleClass": "beans.service.RemovePlan",
			  "requestBody": {
				  "first_level_service_name": g_service_parent,
				  "second_level_service_name": g_service_name,
				  "remove_ips": remove_ips,
			  }
		  };

		  $.post("/JsonRPCServlet",
				  {request_string: JSON.stringify(request)},
				  function (data, status) {
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  showTips("任务已成功创建，请前往任务列表查看任务进度。");
							  getServiceDetail();
						  }
						  else if (data.status == 99) {
							  document.location.href = "/pages/users/login.jsp";
						  }
						  else {
							  showTips(data.message);
						  }
					  }
					  else {
						  showTips(status);
					  }
				  });
	  }

	  function onQueryMonitorClicked()
	  {
		  first_level_service_name = encodeURI(g_service_parent);
		  second_level_service_name = encodeURI(g_service_name);

		  var url = "/pages/monitor/Monitor.jsp?service=" + second_level_service_name + "&service_parent="+first_level_service_name;
		  //  alert(url);
		  //$("#right").load(url);
		  window.open(url);
		  return false;
	  }
	  $(document).ready(function(){
		  getServiceDetail();// ajax这些函数的回调可能晚于下面的代码哈
		  $("#dt_begin").datepicker({ changeMonth: true, changeYear: true, maxDate: 0, dateFormat: 'yy-mm-dd',
			  onClose: function( selectedDate ) {
				  $( "#dt_end" ).datepicker( "option", "minDate", selectedDate );
			  }});
		  $("#dt_end").datepicker({ changeMonth: true, changeYear: true, maxDate: 0, dateFormat: 'yy-mm-dd',
			  onClose: function( selectedDate ) {
				  $( "#dt_begin" ).datepicker( "option", "maxDate", selectedDate );
			  }});
		  var date = new Date();
		  $("#dt_end").datepicker('setDate', date);
		  $("#dt_begin").datepicker('setDate', new Date(date.getFullYear(), date.getMonth(), 1));
	  });
  </script>
</head>
<div>

<div id="tabs">
	<ul>
		<li><a href="#service_detail">实例列表</a></li>
		<li><a href="#plan_list">历史任务列表</a></li>
		<li><a href="#cluster_plan">容量任务管理</a></li>
	</ul>
	<div id="service_detail">
		<div>
			<button type="button" clas="btn-small" style="font-size: 8px" onclick="getESClusterDetail()">实例状态刷新</button>
			<button type="button" clas="btn-small" style="font-size: 8px" onclick="onQueryMonitorClicked()">整体监控查看</button>
			<span class="health_message" id="health_message"></span>
		</div>
		<div class="form_style" id="div_ip_port_list">
			<table id="table_ip_port_list" class="table table-hover">
			</table>
		</div>
		<div id="plan_status" onclick="onGetPlanDetail(plan_id, true)" hidden="true" style="text-align:center;cursor:pointer;color:red">当前有任务执行，点击查看详情</div>
	</div>

	<!--------------------------------------------------------------------------------->

	<div id="plan_list">
		<div>
			<label for="dt_begin">开始时间:</label>
			<input type="text" id="dt_begin" placeholder="默认本月1日" class="dt_text" >
			<label for="dt_end">结束时间:</label>
			<input type="text" id="dt_end" placeholder="默认今天" class="dt_text" >
			<button type="button" class="btn-small" id="btn_query" onclick="onGetPlanListBtnClicked()">查询</button>
		</div>
		<br/>
		<div id="div_plan_list_result" >
			<table id="table_plan_list" class="table table-hover">
			</table>
		</div>
	</div>

	<!--------------------------------------------------------------------------------->

	<div id="cluster_plan">
		<div id="cluster_plan_tabs">
			<ul>
				<li><a href="#add_svrs">扩容</a></li>
				<li><a href="#remove_svrs">缩容</a></li>
			</ul>
			<div id="add_svrs">
				<div >
					<label for="new_ip"  >新增IP:</label>
					<input type="text" class="" id="new_ip"  style="width:600px;display: inline-block" placeholder="支持批量最多100个IP，英文分号分割">
					<br/>
					<label for="data_dir"  >日志存储路径:</label>
					<input type="text" class="" id="data_dir"  style="width:300px;display: inline-block" value="/esdata">
					<br/>
					<button type="button" class="" id="btn_new_ip" onclick="onNewIpAddBtnClicked()">扩容方案预览</button>
				</div>
				<br/>
				<div id="div_new_ip_table" hidden="true">
					<label for="new_ip_table"  >待扩容IP情况:</label>
					<table class="table table-hover" id="new_ip_table">
						<thead>
						<td>IP</td><td>服务情况</td>
						</thead>
						<tbody id="ip_table_body">
						</tbody>
					</table>
					<button type="button" class="" id="btn_new_ip_table" onclick="onNewIpTableAddBtnClicked()">创建扩容任务</button>
				</div>
				<div id="plan" title="任务进度">
					<table class="table table-hover" id="table_plan">
					</table>
				</div>
			</div>

			<!--------------------------------------------------------------------------------->

			<div id="remove_svrs">
				<div >
					<label for="remove_ip"  >待下线IP:</label>
					<input type="text" class="" id="remove_ip"  style="width:300px;display: inline-block" placeholder="支持批量最多100个IP，英文分号分割">
					<br/>
					<button type="button" class="" id="btn_remove_ip" onclick="onRemoveIpBtnClicked()">缩容方案预览</button>
					<br/>
					<div id="div_remove_ip_table" hidden="true">
						<table class="table table-hover" id="remove_ip_table">
							<thead>
							<td>待下线IP:(二次确认)</td>
							</thead>
							<tbody id="remove_ip_table_body">
							</tbody>
						</table>
						<button type="button" class="" id="btn_remove_ip_table" onclick="onRemoveIpTableBtnClicked()">创建缩容任务</button>
					</div>
				</div>
			</div>
		</div>
	</div>
</div>

<div id="result_message"></div>
</div>


<div style="display:none;width:100px;margin:0 auto;position:fixed;left:45%;top:45%;" id="div_loading">
	<img src="/imgs/progress.gif" />
</div>

</body>
</html>
