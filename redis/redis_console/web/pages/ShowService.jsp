
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
	  var instance_memory = 900;

	  var install_pre;
	  var plan_id;
	  var copy_num;
	  var memory_per_instance;
	  var warning = "";
	  var refresh_id ;
	  var service_set_map;
	  var added_servers;

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
		  $("#new_copy").val('');
		  $("remove_set_id").val('');
		  $("#plan_status").hide();
		  $("#div_new_ip_table").hide();
		  $("#div_remove_ip_table").hide();

		  isInstall = true;
		  isRemoveInstall = true;
		  isRecoverInstall = true;
	  }

	  function getRedisClusterDetail()
	  {
		  hosts = [];
		  if($("#table_ip_port_list").DataTable().rows().data().length > 0) {
			  hosts=$("#table_ip_port_list").DataTable().column(3).data().toArray();
		  }
		  else {
			  console.log("getclusterdetail empty");
			  return;
		  }
		  $("#table_ip_port_list").DataTable().rows().every( function ( rowIdx, tableLoop, rowLoop ) {
			  var d = this.data();
			  d[4] = "";
			  d[5] = "";
			  d[6] = "";
			  this.invalidate();
		  } );
		  $("#recover_old_host").empty();
		  $("#div_recover_message").show();
		  $("#div_recover_process").hide();
		  warning = "";
		  $("#warning_icon").css('display','none');

		  var   request={
			  "handleClass":"beans.service.QueryRedisClusterDetail",
			  "requestBody": {
				  "first_level_service_name":g_service_parent,
				  "second_level_service_name":g_service_name,
				  "hosts":hosts},
		  };
		  $.post("/JsonRPCServlet",
				  {request_string:JSON.stringify(request)},
				  function(data, status) {
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  if(Object.keys(data.info_map).length != hosts.length) {
								  showTips("get status error.");
							  }
							  else
							  {
								  $("#table_ip_port_list").DataTable().rows().every( function ( rowIdx, tableLoop, rowLoop ) {
									  //close child
									  if ( this.child.isShown() ) {
										  // This row is already open - close it
										  this.child.hide();
										  $(this.node()).removeClass('shown');
									  }

									  var d = this.data();
									  info = data.info_map[d[3]];
									  if(info == null) {
										  d[4] = "-";
										  d[5] = "-";
										  d[6] = "ERROR";
									  }
									  else {
										  if (info.ok) {
											  if(info.master) {
												  d[4] = "Master";
												  d[5] = "-";
											  }
											  else {
												  d[4] = "Slave";
												  d[5] = info.seq;
											  }
										  }
										  else {
											  d[4] = "-";
											  d[5] = "-";
										  }
										  d[6] = info.status;
									  }

									  if(info == null || !info.ok) {
										  if(info == null || info.status != "Syncing") {
											  $("#div_recover_message").hide();
											  $("#div_recover_process").show();
											  $("#recover_old_host").append($("<option></option>").text(d[3]));
										  }
										  this.nodes().to$().find('td:eq(6)').css("color", "red");
										  this.nodes().to$().find('td:eq(6)').css("fontWeight", "bold");
									  }
									  else {
										  this.nodes().to$().find('td:eq(6)').css("color", "green");
										  this.nodes().to$().find('td:eq(6)').css("fontWeight", "normal");
									  }

									  this.invalidate();
								  } );
								  $("#table_ip_port_list").DataTable().draw();
							  }
							  if(data.warning != null) {
								  warning = data.warning;
								  $("#warning_icon").css('display','inline');
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

	  function RedisCmd(host, cmd)
	  {
		  var   request={
			  "handleClass":"beans.service.RedisCmd",
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
							  if(/^cluster/.test(cmd)) {
								  getServiceDetail();
							  }
							  if(data.detail != "") {
								  $("<div id='redis_detail'></div>").html(data.detail)
										  .dialog({
											  resizable: false,
											  minWidth: 500,
											  maxHeight: 640,
											  modal: true,
											  title: 'Info',
											  hide: 'scale',
											  buttons: {
												  OK: function() {
													  $(this).dialog( "close" );
												  }
											  },
											  close: function(e) {
												  e.preventDefault();
												  $(this).remove();
											  }
										  });
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
				  }
		  );
	  }

	  function InfoOP(d) {
		  RedisCmd(d[3], "info");
	  }

	  function FailoverOP(d) {
		  console.log(d[3]);
		  var r = confirm("确定将此从机提升为主机？当前数据同步差异为"+d[5]);
		  if(r == true) {
			  RedisCmd(d[3], "cluster failover");
		  }
	  }

	  function detail_op_format(d)
	  {
		  var out = '<table cellpadding="5" cellspacing="0" border="0" style="padding-left:50px;"><tr>';
		  if(d[6] == "OK") {
			  out += '<td><button type="button" clas="btn-small" style="float: right;font-size: 8px" onclick=\'InfoOP('+JSON.stringify(d)+')\'>实例信息</button></td>';
			  if(d[4] == "Slave") {
				  out += '<td><button type="button" clas="btn-small" style="float: right;font-size: 8px" onclick=\'FailoverOP('+JSON.stringify(d)+')\'>主从切换</button></td>';
			  }
		  }
		  out += '</tr></table>';
		  return out;
	  }

	  function getServiceDetail()
	  {
		  clearInput();
		  $( "#tabs" ).tabs( "option", "active", 0 );

		  service_set_map = {};
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
							  instance_type = data.cluster_info.memory_per_instance/instance_memory;
							  copy_num = data.cluster_info.copy_num;
							  plan_id = data.cluster_info.plan_id;

							  $.each(data.servers, function (i, rec) {
								  var arr_ip =[];
								  arr_ip.push("", rec.set_id, rec.group_id, rec.ip+":"+rec.port, "", "", "");
								  arr_ips.push(arr_ip);
								  if(!service_set_map[rec.set_id])
								  {
									  service_set_map[rec.set_id] = [];
								  }
								  if($.inArray(rec.ip, service_set_map[rec.set_id]) == -1)
									  service_set_map[rec.set_id].push(rec.ip);
							  });

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
										  {title: "Set名"},
										  {title: "组ID"},
										  {
											  "className": 'ip-port',
											  "title": "IP:Port",
										  },
										  {title: "主/从"},
										  {title:"从机同步偏移差"},
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

								  $('#table_ip_port_list tbody').on('click', 'td.ip-port', function () {
									  var tr = $(this).closest('tr');
									  var host = $("#table_ip_port_list").DataTable().row(tr).data()[3];

									  first_level_service_name = encodeURI(g_service_parent);
									  second_level_service_name = encodeURI(g_service_name);

									  var url = "/pages/monitor/Monitor.jsp?service=" + second_level_service_name + "&service_parent="+first_level_service_name+"&host="+host;
									  window.open(url);
								  } );

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

							  getRedisClusterDetail();

							  if(copy_num>0) {
								  //console.log(copy_num);
								  $("#new_copy").val(copy_num);
								  $("#new_copy").attr("disabled", "disabled");
							  }
							  if(instance_type>0) {
								  $('#instance_type').val(instance_type).change();
								  $("#instance_type").attr("disabled", "disabled");
							  }

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
								  arr_plan.push(rec.set_id, rec.group_id, rec.ip+":"+rec.port, (rec.master ? "Master" : "Slave"), rec.status);
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
										  {title: "Set名"},
										  {title: "组ID"},
										  {title: "IP:Port"},
										  {title: "主/从"},
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
		  var   new_ip= $("#new_ip").val();
		  if (!/^[0-9;\.]+$/.test(new_ip))
		  {
			  showTips("ip输入错误");
			  return;
		  }
		  var new_copy = parseInt($("#new_copy").val());
		  if(isNaN(new_copy) || new_copy > 5 || new_copy < 2)
		  {
			  showTips("拷贝数输入错误");
			  return;
		  }

		  if(new_copy > new_ip.split(';').length)
		  {
			  showTips("拷贝数不能多于提供的机器数");
			  return;
		  }

		  var instance_type = parseInt($("#instance_type").val());

		  var   request={
			  "handleClass":"beans.service.AddSecondLevelServiceIPInfo",
			  "requestBody": {"ip": new_ip,
				  "first_level_service_name":g_service_parent,
				  "second_level_service_name":g_service_name,
				  "copy": new_copy,
				  "instance_type": instance_type,
			  }
		  };

		  $.post("/JsonRPCServlet",
				  {request_string:JSON.stringify(request)},
				  function(data, status) {
					  if (status == "success") {//http通信返回200
						  if (data.status == 0) {//业务处理成功
							  if(data.message == "success" && data.added_ips.length > 0) {
								  install_pre = "ok";
								  added_servers = [];
							  }
							  $("#div_new_ip_table").show();
							  $("#ip_table_body").empty();
							  $.each(data.added_ips, function(i, rec){
								  var str = "<tr><td>"+rec.ip+"</td><td>";
								  if(rec.status_message == "ok") {
									  str += "CPU：" + rec.cpu_cores + "核，" + rec.cpu_mhz +"Mhz，当前负载"+ rec.cpu_load +"%<br/>可用内存：" + rec.memory_avail + "MB</td><td>";
									  $.each(rec.servers, function(j,svr){
										  str += "Set名："+svr.set_id+"，组"+svr.group_id+"，端口："+svr.port+"，内存容量:"+svr.memory+"MB，" + (svr.master == true ? "<strong>Master</strong>":"Slave") + "<br/>";
									  })
									  if(rec.servers != null) {
										  added_servers.push.apply(added_servers, rec.servers);
										  if (rec.servers.length != rec.instance_num) {
											  str += "<strong>注意：由于Set内机器性能不一致，当前机器容量未用完，机器可安装" + rec.instance_num + "个实例，实际安装了" + rec.servers.length + "个实例</strong>";
										  }
									  }
									  str += "</td>";
								  }
								  else {
									  install_pre = "error";
									  if (rec.status_message == "memory") {
										  str += "CPU：" + rec.cpu_cores + "核，" + rec.cpu_mhz + "Mhz，当前负载" + rec.cpu_load + "%<br/><strong style=\"color:red\">可用内存不足：" + rec.memory_avail + "MB</strong></td>";

									  }
									  else if(rec.status_message == "cpu"){
										  str += "CPU：" + rec.cpu_cores + "核，" + rec.cpu_mhz + "Mhz，<strong style=\"color:red\">当前负载过重：" + rec.cpu_load + "%</strong><br/>可用内存：" + rec.memory_avail + "MB</td>";
									  }
									  else if(rec.status_message = "shell") {
										  str += "<strong style=\"color:red\">无法连接，请检查IP及是否安装了Remote Shell</strong>";
									  }
									  str +="<td></td>";
								  }
								  str += "</tr>";
								  $("#ip_table_body").append(str);
								  copy_num = new_copy;
								  memory_per_instance = instance_type * instance_memory;
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
			  showTips("请重新确认分配方案");
			  return;
		  }
		  install_pre = "init";

		  var request = {
			  "handleClass": "beans.service.InstallPlan",
			  "requestBody": {
				  "first_level_service_name": g_service_parent,
				  "second_level_service_name": g_service_name,
				  "added_servers": added_servers,
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

	  function onRemoveIpBtnClicked() {
		  var remove_set_id = $("#remove_set_id").val();
		  console.log(remove_set_id);
		  var arr = service_set_map[remove_set_id];
		  if(!arr){
			  showTips("Set名输入非法");
			  return;
		  }
		  plan_id = "";
		  $("#div_remove_ip_table").show();
		  var arrLength = arr.length;
		  $("#remove_ip_table_body").empty();
		  var str ="";
	      for (var i = 0; i < arrLength; i++) {
			  str += "<tr><td>" + arr[i] + "</td></tr>\r\n";
		  }
		  $("#remove_ip_table_body").append(str);
	  }

	  function onRemoveIpTableBtnClicked() {
		  refresh_id = 0;
		  var remove_set_id = $("#remove_set_id").val();
		  if(!remove_set_id || 0 === remove_set_id.length) {
			  showTips("Set名输入错误");
			  return;
		  }

		  var request = {
			  "handleClass": "beans.service.RemovePlan",
			  "requestBody": {
				  "first_level_service_name": g_service_parent,
				  "second_level_service_name": g_service_name,
				  "set_id": remove_set_id,
			  }
		  };

		  isRemoveInstall = false;
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

	  function onRecoverBtnClicked()
	  {
		  var old_host=$("#recover_old_host").val();
		  var new_ip= $("#recover_new_ip").val();
		  if (!/^[0-9;\.]+$/.test(new_ip))
		  {
			  showTips("ip输入错误");
			  return;
		  }
		  var request={
			  "handleClass":"beans.service.RecoverPlan",
			  "requestBody": {
				  "first_level_service_name":g_service_parent,
				  "second_level_service_name":g_service_name,
				  "old_host":old_host,
			      "new_ip":new_ip},
		  };
		  $.post("/JsonRPCServlet",
				  {request_string:JSON.stringify(request)},
				  function(data, status) {
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
					  else
					  {
						  showTips(status);
						  return;
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
		  var instance_type = parseInt($("#instance_type").val());
		  $("#instance_type_note").text("单个Redis实例设置最大内存为"+instance_memory*instance_type+"MB");
	  });

	  $("#refresh_icon").click(function() {
		  getRedisClusterDetail();
	  });
	  $("#warning_icon").click(function() {
		  showTips(warning);
	  });

	  $( "#instance_type" ).change(function() {
		  var instance_type = parseInt($("#instance_type").val());
		  $("#instance_type_note").text("单个Redis实例设置最大内存为"+instance_memory*instance_type+"MB");
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
			<button type="button" clas="btn-small" style="font-size: 8px" onclick="getRedisClusterDetail()">实例状态刷新</button>
			<button type="button" clas="btn-small" style="font-size: 8px" onclick="onQueryMonitorClicked()">整体监控查看</button>
			<span class="icon-warning" id="warning_icon" hidden="true"></span>
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
				<li><a href="#add_set">扩容（新增Set）</a></li>
				<li><a href="#remove_set">缩容（回收Set）</a></li>
				<li><a href="#recover_host">死机恢复/机器替换</a></li>
			</ul>
			<div id="add_set">
				<div >
					<label for="new_ip"  >新增IP:</label>
					<input type="text" class="" id="new_ip"  style="width:600px;display: inline-block" placeholder="支持批量最多100个IP，英文分号分割，新增IP数量必须是拷贝数的整数倍">
					<br/>
					<label for="new_copy"  >拷贝数:</label>
					<input type="text" class="" id="new_copy"  style="width:300px;display: inline-block" placeholder="拷贝数（2-4）">
					<br/>
					<label for="instance_type"  >实例类型:</label>
					<select id="instance_type">
						<option value="1">Small</option>
						<option selected="selected" value="2">Normal</option>
						<option value="4">Huge</option>
					</select>
					<label id="instance_type_note"></label>
					<br/>
					<button type="button" class="" id="btn_new_ip" onclick="onNewIpAddBtnClicked()">分配方案预览</button>
				</div>
				<br/>
				<div id="div_new_ip_table" hidden="true">
					<label for="new_ip_table"  >IP分配方案:</label>
					<table class="table table-hover" id="new_ip_table">
						<thead>
						<td>IP</td><td>当前服务情况</td><td>分配方案</td>
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

			<div id="remove_set">
				<div >
					<label for="remove_set_id"  >待缩容Set名:</label>
					<input type="text" class="" id="remove_set_id"  style="width:300px;display: inline-block" placeholder="">
					<br/>
					<button type="button" class="" id="btn_remove_ip" onclick="onRemoveIpBtnClicked()">缩容方案预览</button>
					<br/>
					<div id="div_remove_ip_table" hidden="true">
						<table class="table table-hover" id="remove_ip_table">
							<thead>
							<td>待回收的IP</td>
							</thead>
							<tbody id="remove_ip_table_body">
							</tbody>
						</table>
						<button type="button" class="" id="btn_remove_ip_table" onclick="onRemoveIpTableBtnClicked()">创建缩容任务</button>
					</div>
				</div>
			</div>

			<!--------------------------------------------------------------------------------->

			<div id="recover_host">
				<div id="div_recover_process" hidden="true">
					<label for="recover_old_host"  >待恢复的Redis组:</label>
					<select id="recover_old_host">
					</select>
					<br/>
					<input type="text" class="" id="recover_new_ip"  style="width:300px;display: inline-block" placeholder="请输入替换机器IP，采用和下线服务一致端口">
					<br/>
					<button type="button" class="" id="btn_recover" onclick="onRecoverBtnClicked()">创建恢复/替换任务</button>
				</div>
				<div id="div_recover_message">服务正常，无机器需替换</div>
				<div class="clear"></div>
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
