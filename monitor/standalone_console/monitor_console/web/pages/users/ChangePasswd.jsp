
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
    <link rel="stylesheet" type="text/css" href="/css/bootstrap.min.css"/>
    <link rel="stylesheet" href="/css/main.css">
    <link rel="stylesheet" href="/css/jquery-ui.css">

    <script type="text/javascript" src="/js/jquery-2.2.0.min.js"></script>
    <script type="text/javascript" src="/js/jquery.form.js"></script>
    <script type="text/javascript" src="/js/jquery-ui.js"></script>
    <script type="text/javascript" src="/js/jquery.cookie.js"></script>
    <script type="text/javascript" src="/js/md5-min.js"></script>
    <script type="text/javascript" src="/js/tea.js"></script>
  <style>
    #title_bar{width:auto;height:auto;   margin-top: 5px }
    #query_form{width:auto;height:auto;  padding:20px;margin-top: 5px;}
    #result_message{width:auto;height:auto;  margin-top: 5px;}

  </style>

  <script type="text/javascript">




    function onChangePassword(ChangePassFunc){

        var   staff_name= $("#staff_name").val();

        if (staff_name == null || staff_name.length < 1  )
        {
            showTips("input can NOT be empty.");
            return;
        }


        var   request={
            "handleClass":"beans.service.GetSalt",
            "requestBody": {"staff_name": staff_name},
        };
        $.post("/JsonRPCServlet",
                {request_string:JSON.stringify(request)},

                function(data, status){
                    if (status == "success") {//http通信返回200
                        if (data.status == 0) {//业务处理成功

                            var salt = data.salt;
                            var challenge = data.challenge;
                            ChangePassFunc(salt, challenge);
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

    }
     function doChangePassword(salt, challenge){

        var   staff_name= $("#staff_name").val();
        var   password = $("#password").val();
         var newPassword = $("#new_password").val();
         if (staff_name == null || staff_name.length < 1 ||
                password == null || password.length < 1 ||
                 newPassword == null || newPassword.length < 1)
        {
          showTips("input can NOT be empty.");
          return;
        }

         // current password
        var password1 = hex_md5(password);

         var password2 = hex_md5(""+password1+salt);
         console.log("password1:", password1, "len:", password1.length);
         console.log("password2:", password2, "len:", password2.length);
         var toEncrypt = ""+password1+challenge;
         console.log("to encryp:", toEncrypt, "len:", toEncrypt.length);
         var encStr = encrypt(toEncrypt, password2)
         console.log("encStr   :", encStr, "len:", encStr.length);
         var decStr = decrypt(encStr, password2);
         console.log("decStr   :", decStr, "len:", decStr.length);

         //new password
         password1 = hex_md5(newPassword);
         newPassword = hex_md5(""+password1+salt);



        var   request={
          "handleClass":"beans.service.ChangePassword",
          "requestBody": {"staff_name": staff_name,
                            "tgt":encStr,
                            "new_password":newPassword},
        };
        $.post("/JsonRPCServlet",
                {request_string:JSON.stringify(request)},

                function(data, status){
                  if (status == "success") {//http通信返回200
                    if (data.status == 0) {//业务处理成功

                        var str="<p>success, 即将跳转到登录页...</p>";

                        $("#result_message").empty();
                        $(str).appendTo("#result_message");
                        $.cookie("msec_user", null,{expires:0,path:"/"});
                        $.cookie("msec_ticket", null, {expires:0,path:"/"});

                        setTimeout("document.location.href = '/pages/users/login.jsp';", 3000);


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

      }
    $(document).ready(function(){
        $("#staff_name").val($.cookie("msec_user"));
    });






  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;用户改密</p>
</div>

<div id="query_form" class="form_style">
  <form class="form-inline">
    <div class="form-group">

      <label for="staff_name">登录名:</label>
      <input type="text" class="form-control" id="staff_name" maxlength="16">
    </div>
    <div class="form-group">
      <label for="password">密码:</label>
      <input type="password" class="form-control" id="password" maxlength="16">
    </div>
      <div class="form-group">
          <label for="new_password">新密码:</label>
          <input type="password" class="form-control" id="new_password" maxlength="16">
      </div>
    <br>
    <br>
    <button type="button" class="btn-small" id="btn_changePass" onclick="onChangePassword(doChangePassword)">改密</button>


  </form>

</div>

<div id="result_message"  class="result_msg_style">


</div>
</body>
</html>
