
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
<%
    String service = request.getParameter("service");
    String service_parent = request.getParameter("service_parent");
%>
<head>
    <title></title>

  <style>
    #title_bar{width:auto;height:auto;   }

    #result_message{width:auto;height:auto; }

  </style>
    <script type="application/javascript" src="/js/ProgressTurn.js"/>
  <script type="text/javascript">
      var g_service_name = "<%=service%>";
      var g_service_parent = "<%=service_parent%>";



      function getServiceDetail()
      {
          //console.log("begin to load detail info...");
          var   request={
              "handleClass":"beans.service.QueryOddSecondLevelServiceDetail",
              "requestBody": {
                  "second_level_service_name": g_service_name,
                  "first_level_service_name": g_service_parent                 },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){

                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              $("#first_level_service_name").empty();
                              $("#first_level_service_name").append(g_service_parent);

                              $("#second_level_service_name").empty();
                              $("#second_level_service_name").append(g_service_name);



////////////////////////////////////////////////////////////////////////////////////////////////////////
                              var str = "";
                              $.each(data.ipList, function (i, rec) {
                                  //ipinfo = rec.ip+","+rec.port+","+rec.status;
                                  ipinfo = {"ip":rec.ip, "port":rec.port, "status":rec.status,"comm_proto":rec.comm_proto};
                                  str += "<option value ='"+JSON.stringify(ipinfo)+"'>"+i+" "+JSON.stringify(ipinfo)+"</option>\r\n";
                                  //<option value ="1">Volvo</option>

                              });
                              //console.log(str);
                              $("#ip_port_list").empty();
                              $(str).appendTo("#ip_port_list");


////////////////////////////////////////////////////////////////////////////////////////////////////////

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
      function onIpPortDelBtnClicked()
      {

          var   ipinfostr= $("#ip_port_list").val();//这是个字符串数组
          if (ipinfostr == null || ipinfostr.length<1)
          {
              showTips("请选择想要删除的IP/port");
              return;
          }
          //下面要花精力把字符串数组转化为json 对象数组
          var ipList = [];
          for (i = 0; i < ipinfostr.length; ++i)
          {
              var str = ipinfostr[i];
              var ipJson = JSON.parse(str);
              console.log(ipJson);
              if (ipJson.status != "disabled") {
                  showTips("不能删除enabled的IP，请先缩容");
                  return;
              }
              ipList.push(ipJson);
          }


          if (confirm("Delete really?") == false)
          {
              return;
          }


          var   request={
              "handleClass":"beans.service.DelOddSecondLevelServiceIPInfo",
              "requestBody": {
                  "ips":ipList
              },

          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              $.each(ipList, function(i, rec){
                                  var ipinfo = {"ip":rec.ip, "port":rec.port, "status":rec.status};
                                  $("#ip_port_list option[value='"+JSON.stringify(ipinfo)+"']").remove();
                              })

                              showTips("删除成功");

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
      function onNewIpPortAddBtnClicked()
      {
          console.log("clicked!")
          var   new_ip= $("#new_ip").val();
          var comm_proto = $("#new_comm_proto").val();

          if (new_ip == null || new_ip.length < 1)
          {
              showTips("IP/port不能为空");
              return;
          }
          if (!/^[0-9\.:;]+$/.test(new_ip))
          {
              showTips("ip/port输入错误");
              return;

          }
          var   request={
              "handleClass":"beans.service.AddOddSecondLevelServiceIPInfo",
              "requestBody": {"ip": new_ip,
                  "comm_proto": comm_proto,
                  "first_level_service_name":g_service_parent,
                "second_level_service_name":g_service_name},
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              $.each(data.addedIPs, function(i, rec){
                                  var ipinfo = {"ip":rec.ip, "port":rec.port, "status":"disabled", "comm_proto":comm_proto};
                                  var str = "<option value ='"+JSON.stringify(ipinfo)+"'>"+JSON.stringify(ipinfo)+"</option>\r\n";
                                  $("#ip_port_list").append(str);
                              })

                              showTips("添加成功");

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
      function onIpPortToggleBtnClicked()
      {
          $("#div_ip_port_list").toggle();
          if ($("#div_ip_port_list").is(":hidden"))
          {
              $("#btn_toggle_ip_port_list").text('+');
          }
          else
          {
              $("#btn_toggle_ip_port_list").text('-');

          }

      }
      function onChangeCapacityBtnClicked()
      {
          var url = "/pages/changeCapacitySteps/step1.jsp?service_name="+encodeURIComponent(g_service_name)+"&service_parent="+encodeURIComponent(g_service_parent);
          $("#right").load(url);
      }
      function refresh_capacity()
      {


          var   request={
              "handleClass":"beans.service.RefreshOddCapacity",
              "requestBody": {"first_level_service_name":g_service_parent,
                  "second_level_service_name":g_service_name,
              },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功


                              showTips("刷新到LB成功");

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

      $(document).ready(function(){

        getServiceDetail();// ajax这些函数的回调可能晚于下面的代码哈


    });

  </script>
</head>
<body>



<div id="title_bar" class="title_bar_style">

    <label for="second_level_service_name">服务名:</label>
    <label id="second_level_service_name"></label>
    &nbsp;&nbsp;&nbsp;

    <label for="first_level_service_name"  >上一级服务名:</label>
    <label id="first_level_service_name"  ></label>
    &nbsp;&nbsp;&nbsp;



    <button clas="btn-small" style="float:right" id="btn_refreshLB" onclick="refresh_capacity()">重刷该服务到LB</button>
    <button clas="btn-small" style="float:right" id="btn_release" onclick="onChangeCapacityBtnClicked()">扩缩容...</button>



</div>
<!--------------------------------------------------------------------------------->
    <div class="form_style">
        <button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_ip_port_list" onclick="onIpPortToggleBtnClicked()">-</button>
        <label for="ip_port_list"  >◆◆IP/port列表:</label>
        <div id="div_ip_port_list" style="margin-left: 10px;margin-right: 10px;">
            <div>
                <select id="ip_port_list" size="10"  class="form-control" multiple="multiple">
                    <option value="invalid ip/port">loading...</option>

                </select>
                <button type="button" class="btn-small" id="btn_del_ipport" onclick="onIpPortDelBtnClicked()">delete selected</button>

            </div>
            <br/>
            <div >
                <label for="new_ip"  >新增IP:port</label>
                <input type="text" class="" id="new_ip"  style="width:500px;display: inline-block" placeholder="格式IP:PORT。支持批量，用英文分号分割多个IP:PORT" maxlength="3000">
                <label for="new_comm_proto"  >协议类型</label>
                <select id="new_comm_proto">
                    <option value="udp">udp</option>
                    <option value="tcp">tcp</option>
                    <option value="tcp and udp">tcp and udp</option>
                </select>
                <button type="button" class="" id="btn_new_ipport" onclick="onNewIpPortAddBtnClicked()">add</button>
            </div>
        </div>
    </div>

<!--------------------------------------------------------------------------------->
        <div id="result_message"  class="result_msg_style">
        </div>

</div>


<div style="display:none;width:100px;margin:0 auto;position:fixed;left:45%;top:45%;" id="div_loading">
    <img src="/imgs/progress.gif" />
</div>

</body>
</html>
