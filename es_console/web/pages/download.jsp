
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
</head>
<style>
  #title_bar{width:auto;height:auto;   margin-top: 5px }
</style>
<body>
<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;下载流程：</p>
</div>
<ol>
  <li>下载 <a href="resources/agent.tgz">Agent</a> 并放置到es服务器的/msec/agent/目录里。</li>
  <li>将下载包解压，使用"/msec/agent/start.sh ip"启动agent，其中ip参数为es console的内网IP。</li>
</ol>
</body>
</html>
