
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
      var g_service_name = "<%=service%>";
      var g_service_parent = "<%=service_parent%>";
      console.log(g_service_name, g_service_parent);





      $(document).ready(function(){

          console.log(g_service_name, g_service_parent);

          $("#first_level_service_name").empty();
          $("#first_level_service_name").append(g_service_parent);

          $("#second_level_service_name").empty();
          $("#second_level_service_name").append(g_service_name);






    });


  </script>
</head>
<body>



<div id="title_bar" class="title_bar_style">

    <label for="second_level_service_name">【异构服务的扩缩容&nbsp;step2】&nbsp;&nbsp;服务名:</label>
    <label id="second_level_service_name"></label>
    &nbsp;&nbsp;&nbsp;

    <label for="first_level_service_name"  >上一级服务名:</label>
    <label id="first_level_service_name"  ></label>
    &nbsp;&nbsp;&nbsp;

</div>
<!--------------------------------------------------------------------------------->
    <p>这里展示与LB的交互结果和进度情况, to do...</p>

<!--------------------------------------------------------------------------------->
 <div id="result_message"  class="result_msg_style">
        </div>




</body>
</html>
