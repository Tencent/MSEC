
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
<%
    String service = request.getParameter("service");
    String service_parent = request.getParameter("service_parent");
%>
<head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <title></title>

  <style>
    #title_bar{width:auto;height:auto;   }

    #result_message{width:auto;height:auto; }

  </style>
    <script type="application/javascript" src="/js/ProgressTurn.js"/>
  <script type="text/javascript">
      var g_service_name = "<%=service%>";
      var g_service_parent = "<%=service_parent%>";

      function onReleaseBtnClicked()
      {
          var url="/pages/releaseSteps/step1.jsp?service="+g_service_name+"&service_parent="+g_service_parent;
          console.log("goto:", url);
          $("#right").load(url);
      }
      function onDownloadDevBtnClicked()
      {
          var url="/pages/downloadDevSteps/step1.jsp?service="+g_service_name+"&service_parent="+g_service_parent;
          console.log("goto:", url);
          $("#right").load(url);
      }


      function hideOtherParts(me)
      {
          var div_list={list:["#div_ip_port_list","#div_config", "#div_IDL", "#div_library","#div_sharedobject" ]};
          var btn_list={list:["#btn_toggle_ip_port_list", "#btn_toggle_config", "#btn_toggle_IDL", "#btn_toggle_library", "#btn_toggle_sharedobject"]};
          $.each(div_list.list, function(i, rec){
              if (rec != me)
              {
                  $(rec).hide();
                  $(btn_list.list[i]).text("+");
              }
          })

      }

      function getServiceDetail()
      {
          $("#service_name").empty();
          $("#service_name").append(g_service_parent+"."+g_service_name);

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



                              $("#first_level_service_name_of_new_library").val(g_service_parent);
                              $("#second_level_service_name_of_new_library").val(g_service_name);

                              $("#first_level_service_name_of_new_sharedobject").val(g_service_parent);
                              $("#second_level_service_name_of_new_sharedobject").val(g_service_name);

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
                              str="";
                              $.each(data.configTagList, function (i, rec) {

                                  taginfo = {"tag_name":rec.tag_name, "memo":rec.memo};
                                  str += "<option value ='"+JSON.stringify(taginfo)+"'>"+i+" "+JSON.stringify(taginfo)+"</option>\r\n";


                              });
                                console.log("append to config_version_list:", str);
                              $("#config_version_list").empty();
                              $(str).appendTo("#config_version_list");
////////////////////////////////////////////////////////////////////////////////////////////////////////
                              str="";
                              $.each(data.idltagList, function (i, rec) {

                                  taginfo = {"tag_name":rec.tag_name, "memo":rec.memo};
                                  str += "<option value ='"+JSON.stringify(taginfo)+"'>"+i+" "+JSON.stringify(taginfo)+"</option>\r\n";


                              });
                              console.log("append to IDL_list:", str);
                              $("#IDL_list").empty();
                              $(str).appendTo("#IDL_list");
////////////////////////////////////////////////////////////////////////////////////////////////////////


                              str="";
                              $.each(data.libraryFileList, function (i, rec) {

                                  taginfo = {"file_name":rec.file_name, "memo":rec.memo};
                                  str += "<option value ='"+JSON.stringify(taginfo)+"'>"+i+" "+JSON.stringify(taginfo)+"</option>\r\n";


                              });
                              console.log("append to library_list:", str);
                              $("#library_list").empty();
                              $(str).appendTo("#library_list");
////////////////////////////////////////////////////////////////////////////////////////////////////////

                              str="";
                              $.each(data.sharedobjectTagList, function (i, rec) {

                                  taginfo = {"tag_name":rec.tag_name, "memo":rec.memo};
                                  str += "<option value ='"+JSON.stringify(taginfo)+"'>"+i+" "+JSON.stringify(taginfo)+"</option>\r\n";


                              });
                              console.log("append to sharedobject_list:", str);
                              $("#sharedobject_list").empty();
                              $(str).appendTo("#sharedobject_list");
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
              "handleClass":"beans.service.DelSecondLevelServiceIPInfo",
              "requestBody": {"ipToDel": ipList,
                  "second_level_service_name":g_service_name,
                  "first_level_service_name":g_service_parent},
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              $.each(ipList, function(i, rec){
                                  var ipinfo = {"ip":rec.ip, "port":rec.port, "status":rec.status,"release_memo":rec.release_memo};
                                  $("#ip_port_list option[value='"+JSON.stringify(ipinfo)+"']").remove();
                              })

                             // showTips("删除成功");


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
         // var   new_port= parseInt($("#new_port").val());
          var new_port = 7963;
          if (new_ip == null || new_ip.length < 1 ||
            new_port == null || new_port.length<1)
          {
              showTips("IP/port不能为空");
              return;
          }
          if (!/^[0-9;\.]+$/.test(new_ip) || !/[0-9]+$/.test(new_port))
          {
              showTips("ip/port输入错误");
              return;

          }

          var   request={
              "handleClass":"beans.service.AddSecondLevelServiceIPInfo",
              "requestBody": {"ip": new_ip,
                  "port":new_port,
                  "status": "disabled",
                  "first_level_service_name":g_service_parent,
                "second_level_service_name":g_service_name},
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功


                              $.each(data.addedIPs, function(i, rec){
                                  var ipinfo = {"ip":rec, "port":new_port, "status":"disabled", "release_memo":""};
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
              hideOtherParts("#div_ip_port_list");
          }

      }






          $(document).ready(function(){

       // getServiceDetail();// ajax这些函数的回调可能晚于下面的代码哈


          });



  </script>
</head>
<body>



<div id="title_bar" class="title_bar_style">

    <p>实例管理</p>

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
                <label for="new_ip"  >新增IP:</label>
                <input type="text" class="" id="new_ip"  style="width:600px;display: inline-block" placeholder="支持批量，最多100个IP，英文分号分割">
                <button type="button" class="" id="btn_new_ipport" onclick="onNewIpPortAddBtnClicked()">add</button>
            </div>
        </div>
    </div>

<!--------------------------------------------------------------------------------->

<div id="result_message"></div>

</div>


<div style="display:none;width:100px;margin:0 auto;position:fixed;left:45%;top:45%;" id="div_loading">
    <img src="/imgs/progress.gif" />
</div>

</body>
</html>
