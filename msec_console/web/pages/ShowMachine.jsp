
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
    #result_message{width:auto;height:auto;  margin-top: 5px ;}
    #result_table{width:auto;height:auto;  margin-top: 5px ;}
  </style>
  <script type="text/javascript">
    function add_machine()
    {
      $("#right").load("/pages/AddMachine.jsp");
    }

    function del_machine(machine_name)
    {
      if (!confirm('Are you sure to DELETE '+machine_name+"?"))
      {
        return;
      }
      var   request={
        "handleClass":"beans.service.DelMachine",
        "requestBody": {"machine_name": machine_name},
      };
      $.post("/JsonRPCServlet",
              {request_string:JSON.stringify(request)},

              function(data, status){

                if (status == "success")// http通信返回200
                {
                   if (data.status==0) {
                     $("#table_row_" + machine_name).hide();
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
    }

    $(document).ready(function(){

      //query button clicked event
      $("#btn_query").click(function()
      {
          var   machine_name= $("#machine_name").val();
          var   machine_ip= $("#machine_ip").val();
         console.log(machine_name, machine_ip, $("#machine_ip"));
          var   request={
            "handleClass":"beans.service.QueryMachineList",
            "requestBody": {"machine_name": machine_name, "machine_ip":machine_ip},
          };
        $.post("/JsonRPCServlet",
                {request_string:JSON.stringify(request)},

                function(data, status){
                 if (status == "success") {//http通信返回200
                      if (data.status == 0) {//业务处理成功

                        var str = "";

                        if (data.machine_list.length == 0)
                        {
                          str="<p>no record match.</p>";

                          $("#result_message").empty();
                          $(str).appendTo("#result_message");
                          $("#my_table").empty();
                          return;
                        }

                        $.each(data.machine_list, function (i, rec) {

                          str += "<tr id='table_row_" + rec.machine_name + "'>";
                          str += "<td>" + rec.machine_name + "</td><td>" + rec.machine_ip+"</td>";
                          str += "<td>" + rec.os_version + "</td><td>" + rec.gcc_version+"</td><td>"+ rec.java_version+"</td>";
                          str += "<td><a href='#' onclick='del_machine(\"" + rec.machine_name + "\")'> 删除</a></td>";
                          str += "</tr>\n";
                        });

                        $("#my_table").empty();
                        $("#result_message").empty();
                        $(str).appendTo("#my_table");
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
                   //showTips(status);//http通信失败的信息
                   var str="<p>"+status+"</p>";

                   $("#result_message").empty();
                   $(str).appendTo("#result_message");
                 }

                });

      })

    })




  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;查询开发/编译机</p>
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
  <button type="button" class="btn-small" id="btn_query">查询</button>
  <button type="button" class="btn-small" id="btn_addNew" onclick="add_machine()">新增...</button>

</form>

</div>
<div id="result_message" class="result_msg_style">
</div>

<div id="result_table" class="table_style">

<table class="table table-hover" >
  <thead>
    <td>名字</td><td>IP</td><td>OS版本</td><td>gcc版本</td><td>java版本</td><td>操作</td>
  </thead>
  <tbody id="my_table">
  <!--
  <tr>
    <td><a href="#" onclick="delete()" 删除</td>
  </tr>
  -->
  </tbody>
</table>
  </div>
</body>
</html>
