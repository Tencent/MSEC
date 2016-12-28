
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
<head>
    <title></title>

  <style>
    #title_bar{width:auto;height:auto;   margin-top: 5px }
    #query_form{width:auto;height:auto;  padding:20px;margin-top: 5px;}
    #result_message{width:auto;height:auto;  margin-top: 5px;}

  </style>
    <script type="text/javascript" src="/js/md5-min.js"></script>
  <script type="text/javascript">
      function query_staff()
      {
          $("#right").load("/pages/ShowDevOp.jsp");
      }

    $(document).ready(function(){

      //query button clicked event
      $("#btn_add").click(function()
      {
          var   staff_name= $("#staff_name").val();
          var   staff_phone= $("#staff_phone").val();
          var   password = $("#password").val();
          if (staff_name == null || staff_name.length < 1 ||
                  staff_phone == null || staff_phone.length < 1 ||
                  password == null || password.length < 1 )
          {
              showTips("input can NOT be empty.");
              return;
          }
          if (!/^[a-zA-Z][a-zA-Z0-9_]+$/.test(staff_name))
          {
              showTips("登录名只能使用字母数字下划线且字母开头");
              return;
          }
          password = hex_md5(password);

          var   request={
            "handleClass":"beans.service.AddNewStaff",
            "requestBody": {"staff_name": staff_name, "staff_phone":staff_phone, "password":password},
          };
        $.post("/JsonRPCServlet",
                {request_string:JSON.stringify(request)},

                function(data, status){
                 if (status == "success") {//http通信返回200
                      if (data.status == 0) {//业务处理成功

                       var str="<p>结果：添加成功。</p>";

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
  <p>&nbsp;&nbsp;新增用户</p>
</div>

<div id="query_form" class="form_style">
<form class="form-inline">
  <div class="form-group">

    <label for="staff_name">登录名:</label>
    <input type="text" class="form-control" id="staff_name" maxlength="16">
  </div>
  <div class="form-group">
    <label for="staff_phone">电话:</label>
    <input type="number" class="form-control" id="staff_phone" maxlength="16">
  </div>
    <div class="form-group">
        <label for="password">密码:</label>
        <input type="password" class="form-control" id="password" maxlength="16">
    </div>
    <br>
    <br>
  <button type="button" class="btn-small" id="btn_add">提交</button>
    <button type="button" class="btn-small" id="btn_addNew" onclick="query_staff()">查询...</button>

</form>

</div>

<div id="result_message"  class="result_msg_style">


  </div>
</body>
</html>
