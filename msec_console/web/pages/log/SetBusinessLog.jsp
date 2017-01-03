
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
    <script type="text/javascript" src="/js/tools.js"/>

    <script type="text/javascript" src="/js/jquery.datetimepicker.js"/>
    <link rel="stylesheet" type="text/css" href="/css/jquery.datetimepicker.css"/ >
  <style>
    #title_bar{width:auto;height:auto;   margin-top: 5px }
    #query_form{width:auto;height:auto;  padding:20px;margin-top: 5px;}
    #result_message{width:auto;height:auto;  margin-top: 5px ;}
    #result_table{width:auto;height:auto;  margin-top: 5px ;}
  </style>

  <script type="text/javascript">

      function queryFieldList()
      {



          var   request={
              "handleClass":"beans.service.QueryBusiLogField",
              "requestBody":   {
              }
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {

                      if (status == "success") {

                          if (data.status == 0) {

                              var i;
                              $("#field_list").empty();
                              for (i = 0; i < data.field_list.length; ++i) {
                                  var rec = data.field_list[i];
                                  var fieldinfo = {"field_name": rec.field_name, "field_type": rec.field_type};
                                  var str = "<option value ='" + JSON.stringify(fieldinfo) + "'>" + JSON.stringify(fieldinfo) + "</option>\r\n";
                                  $("#field_list").append(str);
                              }
                          }


                      }
                      else
                      {
                          showTips(status);
                          return;
                      }

                  });
      }



      $(document).ready(function() {

          onToggleBtn1Clicked();
          onToggleBtn2Clicked();
          queryFieldList();




      });




      function hideOtherParts(me)
      {
          var div_list={list:["#div_field_mngr","#div_colorate"]};
          var btn_list={list:["#btn_toggle_1", "#btn_toggle_2"]};
          $.each(div_list.list, function(i, rec){
              if (rec != me)
              {
                  $(rec).hide();
                  $(btn_list.list[i]).text("+");
              }
          })

      }
      function onToggleBtn1Clicked()
      {
          $("#div_field_mngr").toggle();
          if ($("#div_field_mngr").is(":hidden"))
          {
              $("#btn_toggle_1").text('+');
          }
          else
          {
              $("#btn_toggle_1").text('-');
              hideOtherParts("#div_field_mngr")
          }

      }
      function onToggleBtn2Clicked()
      {
          $("#div_colorate").toggle();
          if ($("#div_colorate").is(":hidden"))
          {
              $("#btn_toggle_2").text('+');
          }
          else
          {
              $("#btn_toggle_2").text('-');
              hideOtherParts("#div_colorate")
          }

      }
      function deleteField()
      {

          //拉取文件内容

          var   selectedFieldStr= $("#field_list").val();
          if (selectedFieldStr == null || selectedFieldStr.length<1)
          {
              showTips("请选择需要删除的字段");
              return;
          }


          var selectedField = $.parseJSON(selectedFieldStr);

          var   request={
              "handleClass":"beans.service.DeleteBusiLogField",
              "requestBody":   {
                  "field_name": selectedField.field_name,
                  "field_type": selectedField.field_type,
                  }
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {

                      if (status == "success") {
                          if (data.status == "0") {
                              $("#field_list option[value='" + selectedFieldStr + "']").remove();
                          }
                      }
                      else
                      {
                          showTips(status);
                          return;
                      }

                  });
      }
      function addNewField()
      {
          var   new_field_name= $("#new_field_name").val();
          var   new_field_type= $("#new_field_type").val();
          if (new_field_name == null || new_field_name.length<1||
                  new_field_type == null || new_field_type.length<1 )
          {
              showTips("字段名不能为空");
              return;
          }

          if (!/^[A-Za-z][A-Za-z0-9_]+$/.test(new_field_name))
          {
              showTips("字段名只能由字母数字和下划线组成，且只能由字母开头");
              return;
          }
          var   request={
              "handleClass":"beans.service.AddBusiLogField",
              "requestBody":   {
                  "field_name": new_field_name,
                  "field_type": new_field_type,
              }
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {

                      if (status == "success") {
                          if (data.status == 0) {
                              var fieldinfo = {"field_name": new_field_name, "field_type": new_field_type};
                              var str = "<option value ='" + JSON.stringify(fieldinfo) + "'>" + JSON.stringify(fieldinfo) + "</option>\r\n";
                              $("#field_list").append(str);
                          }

                      }
                      else
                      {
                          showTips(status);
                          return;
                      }

                  });
      }

      function colorSettingTemplate()
      {
          var str = 'FieldName0=uin\n\
FieldValue0=11228491\n\
FieldName1=merchID\n\
FieldValue1=M1234';

          $("#colorate_setting").val(str);
      }



  </script>
</head>
<body>

<div id="title_bar" class="title_bar_style">
  <p>&nbsp;&nbsp;日志设置</p>
</div>

<div class="form_style">

    <p><button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_1" onclick="onToggleBtn1Clicked()">-</button>◆◆字段增删</p>
  <div class="form-group" id="div_field_mngr">
      <select id="field_list" size="5" class="form-control">
          <!--
          <option value="client ip">client ip</option>
          <option value="server ip">server ip</option>
          <option value="RPC name">RPC name</option>
          -->
      </select>
      <button type="button" onclick="deleteField()" hidden="true">delete selected</button>
      <br>
      <br>
      新增字段：<br>
      字段名：<input type="text" id="new_field_name">
      类型：<select id="new_field_type">
      <option value="Integer">int</option>
      <option value='String'>varchar(1024)</option>
      </select>
      <button type="button" onclick="addNewField()">add new</button>&nbsp;<span style="color: red">只能增加不能删除，请慎重</span>
  </div>
</div>

<div class="form_style" >

    <p><button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_2" onclick="onToggleBtn2Clicked()">-</button>◆◆染色设置</p>
    <div class="form-group" id="div_colorate">
        <textarea id="colorate_setting" rows="12" style="resize:none;" class="form-control"
                  placeholder="完善文本框中的配置后，复制粘贴到标准服务的配置文件里，发布出去。"/>
        <button type="button" onclick="colorSettingTemplate()">格式示例</button>
    </div>

</div>

<div id="result_message" class="result_msg_style">
</div>

<div id="tips">
</div>

<div id="result_table" class="table_style">
</div>
</body>
</html>
