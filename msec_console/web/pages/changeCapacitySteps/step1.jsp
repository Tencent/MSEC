
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
    String service = request.getParameter("service_name");
    String service_parent = request.getParameter("service_parent");

%>
<head>
    <title></title>

  <style>
    #title_bar{width:auto;height:auto;   }

    #result_message{width:auto;height:auto; }

  </style>
  <script type="text/javascript">
      var g_ip_list = {};
      var g_service_name = "<%=service%>";
      var g_service_parent = "<%=service_parent%>";
      console.log(g_service_name, g_service_parent);


      function onNextBtnClicked()
      {
          var action_type_group = $("[name='action_type_group']").filter(":checked");
          var action_type = "";
          if (action_type_group.attr("id") == "expand")
          {
              action_type = "expand";
          }
          else
          {
              action_type = "reduce";
          }

          var selectedStr = $("#ip_port_list").val();//这是个字符串数组
          if (selectedStr == null || selectedStr.length < 1)
          {
              showTips("请选择IP");
              return;
          }
          if (action_type == "expand")
          {
              if (!confirm("请确保扩容的机器已经完成了程序部署和启动。继续吗？"))
              {
                  return;
              }
          }
          else
          {
              if (!confirm("请确保缩容后该服务的容量够用。继续吗？"))
              {
                  return;
              }
          }
          //console.log("selected:", selectedStr);
          //下面要花精力把字符串数组转化为json 对象数组
          var ipList = [];
          for (i = 0; i < selectedStr.length; ++i)
          {
                var str = selectedStr[i];
                var ipJson = JSON.parse(str);
                console.log(ipJson);
                ipList.push(ipJson);
          }
         // console.log(ipList);
          var   request={
              "handleClass":"beans.service.ChangeCapacity",
              "requestBody": {
                  "second_level_service_name": g_service_name,
                  "first_level_service_name": g_service_parent,
                  "step_number": 1,
                  "action_type":action_type,
                  "ip_list":ipList
              },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){

                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功


                              var url="/pages/changeCapacitySteps/step2.jsp?service_name="+g_service_name+
                                                            "&service_parent="+g_service_parent;
                              $("#right").load(url);

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
      }


      function getServiceDetail()
      {
          //console.log("begin to load detail info...");
          var   request={
              "handleClass":"beans.service.QuerySecondLevelServiceDetail",
              "requestBody": {
                  "service_name": g_service_name,
                  "service_parent": g_service_parent                 },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){

                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功





////////////////////////////////////////////////////////////////////////////////////////////////////////
                              var str = "";
                              g_ip_list = {};

                              var j = 0;
                              $.each(data.ipList, function (i, rec) {

                                  var ipinfo = {"ip":rec.ip, "port":rec.port, "status":rec.status,"release_memo":rec.release_memo};
                                  if (rec.status == "disabled")
                                  {

                                      str += "<option value ='" + JSON.stringify(ipinfo) + "'>" + j+" "+ JSON.stringify(ipinfo) + "</option>\r\n";
                                      j++;
                                      if ("expand" in g_ip_list)
                                      {
                                          g_ip_list.expand.push(ipinfo);
                                      }
                                      else
                                      {
                                          var arr = [];
                                          arr.push(ipinfo);
                                          g_ip_list.expand = arr;

                                      }
                                  }
                                  else
                                  {



                                      if ("reduce" in g_ip_list)
                                      {
                                          g_ip_list.reduce.push(ipinfo);
                                      }
                                      else
                                      {
                                          var arr = [];
                                          arr.push(ipinfo);
                                          g_ip_list.reduce = arr;

                                      }
                                  }


                              });
                              //console.log(str);
                              $("#ip_port_list").empty();
                              $(str).appendTo("#ip_port_list");



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


      $(document).ready(function(){

          console.log(g_service_name, g_service_parent);

          $("#first_level_service_name").empty();
          $("#first_level_service_name").append(g_service_parent);

          $("#second_level_service_name").empty();
          $("#second_level_service_name").append(g_service_name);


         getServiceDetail();





    });
      function onActionTypeChanged()
      {
          var action_type_group = $("[name='action_type_group']").filter(":checked");
          var iparr = [];
          if (action_type_group.attr("id") == "expand")
          {
              var str = "请选择扩容上架的机器："
              $("#tip_label").empty();
              $("#tip_label").append(str);

              iparr = g_ip_list.expand;
          }
          else
          {
              var str = "请选择缩容下架的机器："
              $("#tip_label").empty();
              $("#tip_label").append(str);

              iparr = g_ip_list.reduce;
          }
          var i;
          var str = "";
          if (iparr != null) {
              for (i = 0; i < iparr.length; ++i) {

                  //var ipinfo = {"ip":rec.ip, "port":rec.port, "status":rec.status};
                  str += "<option value ='" + JSON.stringify(iparr[i]) + "'>"+i+" " + JSON.stringify(iparr[i]) + "</option>\r\n";
              }
          }
          $("#ip_port_list").empty();
          $(str).appendTo("#ip_port_list");

      }

  </script>
</head>
<body>



<div id="title_bar" class="title_bar_style">

    <label for="second_level_service_name">【服务的扩缩容&nbsp;step1】&nbsp;&nbsp;服务名:</label>
    <label id="second_level_service_name"></label>
    &nbsp;&nbsp;&nbsp;

    <label for="first_level_service_name"  >上一级服务名:</label>
    <label id="first_level_service_name"  ></label>
    &nbsp;&nbsp;&nbsp;

</div>
<!--------------------------------------------------------------------------------->
    <div class="form_style">
        <br>
        &nbsp;&nbsp;<input type="radio" id="expand" checked="checked" name="action_type_group" onchange="onActionTypeChanged()" />扩容
        &nbsp;&nbsp;<input type="radio" id="reduce" name="action_type_group" onchange="onActionTypeChanged()" />缩容
        <br>
        <br>
        &nbsp;&nbsp;<label for="ip_port_list"  id="tip_label">请选择扩容上架的机器：</label>
        <div id="div_ip_port_list" style="margin-left: 10px;margin-right: 10px;">
            <div>
                <select id="ip_port_list" size="8"  class="form-control" multiple="multiple">
                </select>
                <button type="button" class="btn-small" id="btn_next" onclick="onNextBtnClicked()">下一步</button>


            </div>
        </div>

    </div>

<!--------------------------------------------------------------------------------->
 <div id="result_message"  class="result_msg_style">
        </div>




</body>
</html>
