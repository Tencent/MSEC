
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
    #result_message{width:auto;height:auto;  margin-top: 5px;}

  </style>
  <script type="text/javascript">
      function query_machine()
      {
          $("#right").load("/pages/ShowMachine.jsp");
      }

    $(document).ready(function(){

      //query button clicked event
      $("#btn_add").click(function()
      {
          var   machine_name= $("#machine_name").val();
          var   machine_ip= $("#machine_ip").val();
          var   os_version= $("#os_version").val();
          var   gcc_version= $("#gcc_version").val();
          var   java_version= $("#java_version").val();

          var   request={
            "handleClass":"beans.service.AddNewMachine",
            "requestBody": {"machine_name": machine_name,
                                "machine_ip":machine_ip,
                                "os_version":os_version,
                                "gcc_version":gcc_version,
                                "java_version":java_version,
                            },
          };
        $.post("/JsonRPCServlet",
                {request_string:JSON.stringify(request)},

                function(data, status){
                 if (status == "success") {//http通信返回200
                      if (data.status == 0) {//业务处理成功

                       var str="<p>结果：添加成功。.</p>";

                        $("#result_message").empty();
                        $(str).appendTo("#result_message");
                      }
                      else if (data.status == 99)
                      {
                          document.location.href = "/pages/users/login.jsp";
                      }
                   else
                      {
                        //showTips(data.message);//业务处理失败的信息
                          var str="<p>"+data.message+"</p>";

                          $("#result_message").empty();
                          $(str).appendTo("#result_message");
                      }
                 }
                  else
                 {
                   showTips(status);//http通信失败的信息
                 }

                });

      })

    })




  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;新增开发/编译机</p>
</div>

<div id="query_form" class="form_style">
<form class="form-inline">
  <div class="form-group">

    <label for="machine_name">机器名:</label>
    <input type="text" class="form-control" id="machine_name" >
  </div>
  <div class="form-group">
    <label for="machine_ip">IP:</label>
    <input type="text" class="form-control" id="machine_ip" >
  </div>
    <div class="form-group">
        <label for="os_version">OS version:</label>
        <input type="text" class="form-control" id="os_version" >
    </div>
    <div class="form-group">
        <label for="gcc_version">GCC version:</label>
        <input type="text" class="form-control" id="gcc_version" >
    </div>
    <div class="form-group">
        <label for="java_version">java version:</label>
        <input type="text" class="form-control" id="java_version" >
    </div>
  <button type="button" class="btn-small" id="btn_add">提交</button>
    <button type="button" class="btn-small" id="btn_addNew" onclick="query_machine()">查询...</button>

</form>

</div>

<div id="result_message"  class="result_msg_style">


  </div>
</body>
</html>
