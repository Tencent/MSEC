
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
      var g_port = 7963;
      var g_dev_lang = "c++";

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

                              $("#dev_lang").append(data.dev_lang);
                              $("#port").append(data.port);
                              g_port = data.port;
                              g_dev_lang = data.dev_lang;

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
          var new_port = g_port;
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
          var comm_proto = "tcp and udp";
          if (g_dev_lang == "java")
          {
              comm_proto = "tcp";
          }

          var   request={
              "handleClass":"beans.service.AddSecondLevelServiceIPInfo",
              "requestBody": {"ip": new_ip,
                  "port":new_port,
                  "comm_proto":comm_proto,
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
      function onConfigToggleBtnClicked()
      {
          $("#div_config").toggle();
          if ($("#div_config").is(":hidden"))
          {
              $("#btn_toggle_config").text('+');
          }
          else
          {
              $("#btn_toggle_config").text('-');
              hideOtherParts("#div_config");
          }

      }
      function onTagListDbClicked()
      {

          //拉取文件内容

          var   selectedTagStr= $("#config_version_list").val();
          if (selectedTagStr == null || selectedTagStr.length<1)
          {
              showTips("请选择需要查看的tag");
              return;
          }


          var selectedTag = $.parseJSON(selectedTagStr);

          var   request={
              "handleClass":"beans.service.DownloadConfigFileContent",
              "requestBody": {
                  "first_level_service_name":g_service_parent,
                  "second_level_service_name":g_service_name,
                    "tag_name":selectedTag.tag_name},
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {

                      if (status == "success") {
                          //console.log(data);
                          $("#config_content").empty();
                          $("#config_content").val(data);
                      }
                      else
                          {
                              showTips(status);
                              return;
                          }

                      });
                  }
      function onConfigTagDelBtnClicked()
      {

          var   selectedTagStr= $("#config_version_list").val();
          if (selectedTagStr == null || selectedTagStr.length<1)
          {
              showTips("请选择需要删除的tag");
              return;
          }


          var selectedTag = $.parseJSON(selectedTagStr);

          if (confirm("Delete really?") == false)
          {
              return;
          }


          var   request={
              "handleClass":"beans.service.DelSecondLevelServiceConfigTag",
              "requestBody":   {
                  "first_level_service_name": g_service_parent,
                    "second_level_service_name": g_service_name,
                    "tag_name":selectedTag.tag_name,}
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              //showTips("删除成功");
                              $("#config_version_list option[value='"+selectedTagStr+"']").remove();
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
      function onNewConfigTagAddBtnClicked()
      {
            var new_tag_name = $("#new_tag_name").val();
          var new_tag_memo = $("#new_tag_memo").val();
          var new_tag_content= $("#config_content").val();

          if (new_tag_name == null || new_tag_name.length<1)
          {
              showTips("tag name不能为空");
              return;
          }
          if (!/^[A-Za-z][A-Za-z0-9_]+$/.test(new_tag_name))
          {
              showTips("tag name 只能由字母数字和下划线组成，且只能由字母开头");
              return;
          }




          var   request={
              "handleClass":"beans.service.AddSecondLevelServiceConfigTag",
              "requestBody":   {
                  "first_level_service_name": g_service_parent,
                  "second_level_service_name": g_service_name,
                  "tag_name":new_tag_name,
                    "memo":new_tag_memo,
                    "content": new_tag_content}
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              showTips("添加成功");
                              taginfo = {"tag_name":new_tag_name, "memo":new_tag_memo};
                              str = "<option value ='"+JSON.stringify(taginfo)+"'>"+JSON.stringify(taginfo)+"</option>\r\n";
                              $("#config_version_list").append(str);
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
      function onIDLGeneCodeBtnClicked()
      {

          var   selectedTagStr= $("#IDL_list").val();
          if (selectedTagStr == null || selectedTagStr.length<1)
          {
              showTips("请选择tag");
              return;
          }


          var selectedTag = $.parseJSON(selectedTagStr);



          var   request={
              "handleClass":"beans.service.IDLGeneCodeAndDownload",
              "requestBody":   {
                  "first_level_service_name": g_service_parent,
                  "second_level_service_name": g_service_name,
                  "tag_name":selectedTag.tag_name,}
          };
          var url = "/JsonRPCServlet?request_string="+JSON.stringify(request);
          window.open(url);


      };
      function onIDLDelBtnClicked()
      {

          var   selectedTagStr= $("#IDL_list").val();
          if (selectedTagStr == null || selectedTagStr.length<1)
          {
              showTips("请选择tag");
              return;
          }


          var selectedTag = $.parseJSON(selectedTagStr);

          if (confirm("Delete really?") == false)
          {
              return;
          }


          var   request={
              "handleClass":"beans.service.DelIDLTag",
              "requestBody":   {
                  "first_level_service_name": g_service_parent,
                  "second_level_service_name": g_service_name,
                  "tag_name":selectedTag.tag_name,}
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              //showTips("删除成功");
                              $("#IDL_list option[value='"+selectedTagStr+"']").remove();
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
      function onIDLListDbClicked()
      {

          //拉取文件内容

          var   selectedTagStr= $("#IDL_list").val();
          if (selectedTagStr == null || selectedTagStr.length<1)
          {
              showTips("请选择需要查看的tag");
              return;
          }


          var selectedTag = $.parseJSON(selectedTagStr);

          var   request={
              "handleClass":"beans.service.DownloadIDLFileContent",
              "requestBody":   {
                  "first_level_service_name": g_service_parent,
                  "second_level_service_name": g_service_name,
                  "tag_name":selectedTag.tag_name,}
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {

                      if (status == "success") {
                          //console.log(data);
                          $("#IDL_content").empty();
                          $("#IDL_content").val(data);
                      }
                      else
                      {
                          showTips(status);
                          return;
                      }

                  });
      }
      function onNewIDLTagAddBtnClicked()
      {
          var new_tag_name = $("#new_IDL_name").val();
          var new_tag_memo = $("#new_IDL_memo").val();
          var new_tag_content= $("#IDL_content").val();

          if (new_tag_name == null || new_tag_name.length<1)
          {
              showTips("tag name不能为空");
              return;
          }
          if (!/^[A-Za-z][A-Za-z0-9_]+$/.test(new_tag_name))
          {
              showTips("tag name 只能由字母数字和下划线组成，且只能由字母开头");
              return;
          }




          var   request={
              "handleClass":"beans.service.AddIDLTag",
              "requestBody":   {
                  "first_level_service_name": g_service_parent,
                  "second_level_service_name": g_service_name,
                  "tag_name":new_tag_name,
                  "memo":new_tag_memo,
                  "content": new_tag_content}
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              showTips("添加成功");
                              var taginfo = {"tag_name":new_tag_name, "memo":new_tag_memo};
                              var str = "<option value ='"+JSON.stringify(taginfo)+"'>"+JSON.stringify(taginfo)+"</option>\r\n";
                              $("#IDL_list").append(str);
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
      function onIDLToggleBtnClicked()
      {
          $("#div_IDL").toggle();
          if ($("#div_IDL").is(":hidden"))
          {
              $("#btn_toggle_IDL").text('+');
          }
          else
          {
              $("#btn_toggle_IDL").text('-');
              hideOtherParts("#div_IDL")
          }

      }

      function onLibraryDelBtnClicked()
      {

          var   selectedStr= $("#library_list").val();
          if (selectedStr == null || selectedStr.length<1)
          {
              showTips("请选择需要删除的文件");
              return;
          }


          var selectedJson = $.parseJSON(selectedStr);

          if (confirm("真的删除吗？") == false)
          {
              return;
          }


          var   request={
              "handleClass":"beans.service.DelLibraryFile",
              "requestBody":   {
                  "first_level_service_name": g_service_parent,
                  "second_level_service_name": g_service_name,
                  "file_name":selectedJson.file_name,}
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              //showTips("删除成功");
                              $("#library_list option[value='"+selectedStr+"']").remove();
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
      function onLibraryToggleBtnClicked()
      {
          $("#div_library").toggle();
          if ($("#div_library").is(":hidden"))
          {
              $("#btn_toggle_library").text('+');
          }
          else
          {
              $("#btn_toggle_library").text('-');
              hideOtherParts("#div_library")
          }

      }
      function onSharedobjectDelBtnClicked()
      {

          var   selectedStr= $("#sharedobject_list").val();
          if (selectedStr == null || selectedStr.length<1)
          {
              showTips("please select the item to delete.");
              return;
          }


          var selectedJson = $.parseJSON(selectedStr);

          if (confirm("真的删除吗？") == false)
          {
              return;
          }


          var   request={
              "handleClass":"beans.service.DelSharedobject",
              "requestBody":   {
                  "first_level_service_name": g_service_parent,
                  "second_level_service_name": g_service_name,
                  "tag_name":selectedJson.tag_name,}
          };

          $.post("/JsonRPCServlet",
                  {request_string:JSON.stringify(request)},
                  function(data, status) {
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              //showTips("删除成功");
                              $("#sharedobject_list option[value='"+selectedStr+"']").remove();
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
      function onSharedobjectToggleBtnClicked()
      {
          $("#div_sharedobject").toggle();
          if ($("#div_sharedobject").is(":hidden"))
          {
              $("#btn_toggle_sharedobject").text('+');
          }
          else
          {
              $("#btn_toggle_sharedobject").text('-');
              hideOtherParts("#div_sharedobject")
          }

      }


      function showLibraryFileUploadResponse(responseText, statusText, xhr, $form)  {

          if (statusText != "success")
          {
             showTips(statusText);
              return;
          }


          var responseJson = $.parseJSON(responseText);
          if (responseJson.status == 0)
          {
              console.log("update library list");
              var library_file_name = $("#new_library_file_name").val();
              var library_memo = $("#new_library_memo").val();
              var str="{"+"\"file_name\":\""+responseJson.file_name+"\", \"memo\":\""+library_memo+"\"}";
              console.log(str);
              var opt= "<option value ='"+str+"'>"+str+"</option>\r\n";
              console.log(opt);

              $("#library_list").append(opt);
              showTips("success");
          }
          else
          {
              showTips(responseJson.message);
          }
      }
      function showSharedobjectUploadResponse(responseText, statusText, xhr, $form)  {

        if (statusText != "success")
        {
            showTips(statusText);
            return;
        }

          var responseJson = $.parseJSON(responseText);
          if (responseJson.status == 0)
          {
              console.log("update sharedobject list");
              var tag_name = $("#new_sharedobject_tag").val();
              var memo = $("#new_sharedobject_memo").val();
              var str="{"+"\"tag_name\":\""+tag_name+"\", \"memo\":\""+memo+"\"}";
              console.log(str);
              var opt= "<option value ='"+str+"'>"+str+"</option>\r\n";
              console.log(opt);

              $("#sharedobject_list").append(opt);
              showTips("success");

          }
          else
          {
              showTips(responseJson.message);
          }
      }
      function showSharedobjectUploadRequest(formData, jqForm, options) {

          console.log("formData", formData);
          console.log("jqForm", jqForm);

          for (i = 0; i < formData.length;++i)
          {
              if (formData[i].name=="new_sharedobject_tag")
              {
                  if (!/^[a-zA-Z][a-zA-Z0-9_]+$/.test(formData[i].value))
                  {
                      showTips("tag只能由字母数字下划线组成，且只能使用字母开头");
                      return false;
                  }
              }
          }


          // here we could return false to prevent the form from being submitted;
          // returning anything other than false will allow the form submit to continue
          return true;
      }

          $(document).ready(function(){

        getServiceDetail();// ajax这些函数的回调可能晚于下面的代码哈

        //都调用一把，把他们收拢
        onIpPortToggleBtnClicked();
        onConfigToggleBtnClicked();
        onIDLToggleBtnClicked();
          onLibraryToggleBtnClicked();
          onSharedobjectToggleBtnClicked();

          //上传库文件的form的异步化处理

          var options = {

              success:       showLibraryFileUploadResponse  // post-submit callback
          };
          $('#library_upload_form').ajaxForm(options);


          //上传so文件的form的异步化处理
          options = {
              beforeSubmit:showSharedobjectUploadRequest,
              success:       showSharedobjectUploadResponse  // post-submit callback
          };
          $('#sharedobject_upload_form').ajaxForm(options);

    });

      function showConfigTemplate()
      {
          var str = "[LOG]\n";
          str += "Level=INFO\n"
          str += "[COLOR]\n";
          str += ";FieldName0=uin\n";
          str += ";FieldValue0=11228491\n";
          str += ";FieldName1=merchID\n";
          str += ";FieldValue1=M1234\n";


          $("#config_content").val(str);
      }
      function showIDLTemplate()
      {
          var str = 'option cc_generic_services = true; \n\
option java_generic_services = true;\n\n\
package xxx;\n\
message FooRequest {\n\
    required string name=1;\n\
    required int32 id=2;\n\
    optional string email=3;\n\
};\n\
message FooResponse {\n\
    required int32 errcode=1;\n\
    required string errmsg=2;\n\
};\n\
service FooService {\n\
    rpc GetSomething(FooRequest) returns (FooResponse);\n\
}';

          $("#IDL_content").val(str);
      }

  </script>
</head>
<body>



<div id="title_bar" class="title_bar_style">

    <label for="service_name">配置管理&nbsp&nbsp服务名:</label>
    <label id="service_name"></label>
    ,&nbsp;

    <label for="dev_lang"  >开发语言:</label>
    <label id="dev_lang"  ></label>
    ,&nbsp;

    <label for="port"  >监听端口:</label>
    <label id="port"  ></label>

    <button clas="btn-small" style="float:right" id="btn_DownloadDev" onclick="onDownloadDevBtnClicked()">获取开发环境...</button>
    <button clas="btn-small" style="float:right" id="btn_release" onclick="onReleaseBtnClicked()">制定发布计划...</button>

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
    <div class="form_style">
    <button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_config" onclick="onConfigToggleBtnClicked()">-</button>
    <label for="div_config"  >◆◆配置文件:</label>
    <div  class="form_style" id="div_config" style="margin-left: 10px;margin-right: 10px;">

        <div>
            <label for="div_config"  >配置文件版本列表:（双击可以查看内容）</label>
            <select id="config_version_list" size="8"  class="form-control" ondblclick="onTagListDbClicked()" >
            </select>
            <button type="button" class="btn-small" id="btn_del_config_version" onclick="onConfigTagDelBtnClicked()">delete selected</button>
        </div>
        <br>
        <!--<div   style="width:50%;float:left; margin-left: 5px;">-->
        <div>
            <label for="config_content"  >配置文件内容:</label>
            <button type="button" style="display:inline-block"   onclick="showConfigTemplate()">格式示例</button>
            <br>
            <textarea class="form-control" id="config_content" rows="12" style="resize:none;"></textarea>

            新tag：<input type="text" style="width:150px;display: inline-block" id="new_tag_name" maxlength="16">
            备注：<input type="text" style="width:250px;display: inline-block" id="new_tag_memo" maxlength="16">

            <button type="button" style="display:inline-block"  id="btn_new_config_version" onclick="onNewConfigTagAddBtnClicked()">保存为新版本</button>
        </div>
        <div class="clear"></div>
    </div>
        </div>

<!--------------------------------------------------------------------------------->

    <div class="form_style">
    <button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_IDL" onclick="onIDLToggleBtnClicked()">-</button>
    <label for="div_IDL"  >◆◆RPC的接口定义文件:</label>
    <div  class="form_style" id="div_IDL" style="margin-left: 10px;margin-right: 10px;">

        <div>
            <label for="IDL_list"  >文件版本列表:（双击可以查看内容）</label>
            <select id="IDL_list" size="5"  class="form-control" ondblclick="onIDLListDbClicked()" >
            </select>
            <button type="button" class="btn-small" id="btn_del_IDL" onclick="onIDLDelBtnClicked()">delete selected</button>
            <button type="button" class="btn-small" id="btn_IDL_gene_code" onclick="onIDLGeneCodeBtnClicked()">下载调用方用的库</button>
        </div>
        <br>

        <div>
            <label for="IDL_content"  >IDL文件内容:</label>
            <button type="button" style="display:inline-block"   onclick="showIDLTemplate()">格式示例</button>
            <br>
            <textarea class="form-control" id="IDL_content" rows="17" style="resize:none;"></textarea>

            新tag：<input type="text" style="width:150px;display: inline-block" id="new_IDL_name" maxlength="16">
            备注：<input type="text" style="width:250px;display: inline-block" id="new_IDL_memo" maxlength="16">
            <button type="button" style="display:inline-block"  id="btn_new_IDL" onclick="onNewIDLTagAddBtnClicked()">保存为新版本</button>
        </div>
        <div class="clear"></div>
    </div>


</div>
<!--------------------------------------------------------------------------------->

<div class="form_style">
    <button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_library" onclick="onLibraryToggleBtnClicked()">-</button>
    <label for="div_library"  >◆◆外部代码库/资源文件:</label>
    <div  class="form_style" id="div_library" style="margin-left: 10px;margin-right: 10px;">

        <div>
            <label for="library_list"  >列表:</label>
            <select id="library_list" size="5"  class="form-control"  >
            </select>
            <button type="button" class="btn-small" id="btn_del_library" onclick="onLibraryDelBtnClicked()">delete selected</button>
        </div>
        <br>

        <div>
           <form action="/FileUpload" method="post"    id="library_upload_form"  enctype="multipart/form-data" accept-charset="utf-8">
            新代码库/资源文件：<input style="width:250px;display: inline-block"  type="file" name="new_library_file_name" size="150" id="new_library_file_name"/>
            备注：<input type="text" style="width:250px;display: inline-block" id="new_library_memo" name="new_library_memo" maxlength="16"><br>
               <input type="hidden" name="first_level_service_name_of_new_library" id="first_level_service_name_of_new_library">
               <input type="hidden" name="second_level_service_name_of_new_library" id="second_level_service_name_of_new_library">
               <input type="hidden" name="handleClass"  value="beans.service.LibraryFileUpload">
            <button type="submit" style="display:inline-block"  id="btn_new_library" >upload file</button>
            </form>
        </div>
        <div class="clear"></div>
    </div>
</div>
<!--------------------------------------------------------------------------------->

<div class="form_style">
    <button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_sharedobject" onclick="onSharedobjectToggleBtnClicked()">-</button>
    <label for="div_sharedobject"  >◆◆业务插件:</label>
    <div  class="form_style" id="div_sharedobject" style="margin-left: 10px;margin-right: 10px;">

        <div>
            <label for="sharedobject_list"  >tag列表:</label>
            <select id="sharedobject_list" size="5"  class="form-control"  >
            </select>
            <button type="button" class="btn-small" id="btn_del_sharedobject" onclick="onSharedobjectDelBtnClicked()">delete selected</button>
        </div>
        <br>

        <div>
            <form action="/FileUpload" method="post"    id="sharedobject_upload_form"  enctype="multipart/form-data">
                新版本插件：<input style="width:250px;display: inline-block"  type="file" name="new_sharedobject_name" size="150" id="new_sharedobject_name" /><br>
                tag：<input type="text" style="width:250px;display: inline-block" id="new_sharedobject_tag" name="new_sharedobject_tag" maxlength="16"><br>
                备注：<input type="text" style="width:250px;display: inline-block" id="new_sharedobject_memo" name="new_sharedobject_memo" maxlength="16"><br>
                <input type="hidden" name="first_level_service_name_of_new_sharedobject"  id="first_level_service_name_of_new_sharedobject">
                <input type="hidden" name="second_level_service_name_of_new_sharedobject" id="second_level_service_name_of_new_sharedobject">
                <input type="hidden" name="handleClass"  value="beans.service.SharedobjectUpload">
                <button type="submit" style="display:inline-block"  id="btn_new_sharedobject" >upload file</button>
            </form>
        </div>
        <div class="clear"></div>
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
