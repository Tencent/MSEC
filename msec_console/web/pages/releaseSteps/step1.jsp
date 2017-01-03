
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


<%@ page import="beans.request.ReleasePlan" %>
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
    String plan_id = request.getParameter("plan_id");
    if (plan_id == null || plan_id.length() < 4)
    {
        plan_id = ReleasePlan.newPlanID();
    }


%>
<head>
    <title></title>

  <style>
    #title_bar{width:auto;height:auto;   }

    #result_message{width:auto;height:auto; }

  </style>
  <script type="text/javascript">
      var g_service_name = "<%=service%>";
      var g_service_parent = "<%=service_parent%>";
      var g_plan_id = "<%=plan_id%>";


      function onNextBtnClicked()
      {
          var release_type = $("#release_type").val();
          if (release_type == null || release_type.length < 1)
          {
              showTips("请选择发布类型");
              return;
          }

          var selectedStr = $("#ip_port_list").val();//这是个字符串数组
          if (selectedStr == null || selectedStr.length < 1)
          {
              showTips("IP不能为空");
              return;
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
              "handleClass":"beans.service.ReleaseStepsGO",
              "requestBody": {
                  "second_level_service_name": g_service_name,
                  "first_level_service_name": g_service_parent,
                  "step_number": 1,
                  "plan_id":g_plan_id,
                  "release_type":release_type,
                  "dest_ip_list":ipList
              },
          };
          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},

                  function(data, status){

                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              if (release_type == "complete") {
                                       var url = "/pages/releaseSteps/step2.jsp?service=" + g_service_name +
                                          "&service_parent=" + g_service_parent +
                                          "&plan_id=" + g_plan_id;
                              }
                              else if (release_type == "only_config") {
                                  var url = "/pages/releaseSteps/only_config.jsp?service=" + g_service_name +
                                          "&service_parent=" + g_service_parent +
                                          "&plan_id=" + g_plan_id;
                              }
                              else if (release_type == "only_library") {
                                  var url = "/pages/releaseSteps/only_library.jsp?service=" + g_service_name +
                                          "&service_parent=" + g_service_parent +
                                          "&plan_id=" + g_plan_id;
                              }
                              else if (release_type == "only_sharedobject") {
                                  var url = "/pages/releaseSteps/only_sharedobject.jsp?service=" + g_service_name +
                                          "&service_parent=" + g_service_parent +
                                          "&plan_id=" + g_plan_id;
                              }
                              console.log("goto:", url);
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
                              $.each(data.ipList, function (i, rec) {
                                  //ipinfo = rec.ip+","+rec.port+","+rec.status;
                                  ipinfo = {"ip":rec.ip, "port":rec.port, "status":rec.status,"release_memo":rec.release_memo};
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


      $(document).ready(function(){

        getServiceDetail();// ajax这些函数的回调可能晚于下面的代码哈

          $("#first_level_service_name").empty();
          $("#first_level_service_name").append(g_service_parent);

          $("#second_level_service_name").empty();
          $("#second_level_service_name").append(g_service_name);

          $("#action_id").empty();
          $("#action_id").append(g_plan_id);


    });

  </script>
</head>
<body>



<div id="title_bar" class="title_bar_style">

    <label for="second_level_service_name">【发布&nbsp;step1】&nbsp;&nbsp;服务名:</label>
    <label id="second_level_service_name"></label>
    &nbsp;&nbsp;&nbsp;

    <label for="first_level_service_name"  >上一级服务名:</label>
    <label id="first_level_service_name"  ></label>
    &nbsp;&nbsp;&nbsp;

    <label for="action_id"  >Plan_ID:</label>
    <label id="action_id"  ></label>

</div>
<!--------------------------------------------------------------------------------->
    <div class="form_style">
        <label for="ip_port_list"  >选择目标机器:</label>
        <div id="div_ip_port_list" style="margin-left: 10px;margin-right: 10px;">
            <div>
                <select id="ip_port_list" size="10"  class="form-control" multiple="multiple">
                </select>



            </div>
            <br>
            <label for="release_type"  >发布类型:</label>
            <div>
                <select id="release_type" size="1" >
                    <option value="complete">完整发布</option>
                    <option value="only_config">只发布配置文件</option>
                    <option value="only_library">只发布外部代码库/资源文件</option>
                    <option value="only_sharedobject">只发布业务插件</option>
                </select>
            </div>
            <br>
            <button type="button" class="btn-small" id="btn_next" onclick="onNextBtnClicked()">下一步</button>

        </div>

    </div>

<!--------------------------------------------------------------------------------->
        <div id="result_message"  class="result_msg_style">
        </div>

</div>


</body>
</html>
