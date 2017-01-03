
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
    String plan_id = request.getParameter("plan_id");
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



      function showTips(str)
      {
          $("#result_message").empty();
          $("#result_message").append(str);
         //本来是不弹对话框窗口的，担心浏览器关闭，但这里页面比较长，不弹担心用户看不到信息
          alert(str);
      };


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
                      //console.log("QuerySecondLevelServiceDetail returns back");
                     // console.log(status, data);
                      if (status == "success") {//http通信返回200
                          if (data.status == 0) {//业务处理成功

                              $("#first_level_service_name").empty();
                              $("#first_level_service_name").append(data.first_level_service_name);
                              g_service_parent = data.first_level_service_name;

                              $("#second_level_service_name").empty();
                              $("#second_level_service_name").append(g_service_name);

                              $("#first_level_service_name_of_new_library").val(g_service_parent);
                              $("#second_level_service_name_of_new_library").val(g_service_name);

                              $("#first_level_service_name_of_new_sharedobject").val(g_service_parent);
                              $("#second_level_service_name_of_new_sharedobject").val(g_service_name);

////////////////////////////////////////////////////////////////////////////////////////////////////////
                              var str = "";
                              $.each(data.ipList, function (i, rec) {
                                  //ipinfo = rec.ip+","+rec.port+","+rec.status;
                                  ipinfo = {"ip":rec.ip, "port":rec.port, "status":rec.status};
                                  str += "<option value ='"+JSON.stringify(ipinfo)+"'>"+JSON.stringify(ipinfo)+"</option>\r\n";
                                  //<option value ="1">Volvo</option>

                              });
                              //console.log(str);
                              $("#ip_port_list").empty();
                              $(str).appendTo("#ip_port_list");


////////////////////////////////////////////////////////////////////////////////////////////////////////
                              str="";
                              $.each(data.configTagList, function (i, rec) {

                                  taginfo = {"tag_name":rec.tag_name, "memo":rec.memo};
                                  str += "<option value ='"+JSON.stringify(taginfo)+"'>"+JSON.stringify(taginfo)+"</option>\r\n";


                              });
                                console.log("append to config_version_list:", str);
                              $("#config_version_list").empty();
                              $(str).appendTo("#config_version_list");
////////////////////////////////////////////////////////////////////////////////////////////////////////
                              str="";
                              $.each(data.idltagList, function (i, rec) {

                                  taginfo = {"tag_name":rec.tag_name, "memo":rec.memo};
                                  str += "<option value ='"+JSON.stringify(taginfo)+"'>"+JSON.stringify(taginfo)+"</option>\r\n";


                              });
                              console.log("append to IDL_list:", str);
                              $("#IDL_list").empty();
                              $(str).appendTo("#IDL_list");
////////////////////////////////////////////////////////////////////////////////////////////////////////


                              str="";
                              $.each(data.libraryFileList, function (i, rec) {

                                  taginfo = {"file_name":rec.file_name, "memo":rec.memo};
                                  str += "<option value ='"+JSON.stringify(taginfo)+"'>"+JSON.stringify(taginfo)+"</option>\r\n";


                              });
                              console.log("append to library_list:", str);
                              $("#library_list").empty();
                              $(str).appendTo("#library_list");
////////////////////////////////////////////////////////////////////////////////////////////////////////

                              str="";
                              $.each(data.sharedobjectTagList, function (i, rec) {

                                  taginfo = {"tag_name":rec.tag_name, "memo":rec.memo};
                                  str += "<option value ='"+JSON.stringify(taginfo)+"'>"+JSON.stringify(taginfo)+"</option>\r\n";


                              });
                              console.log("append to sharedobject_list:", str);
                              $("#sharedobject_list").empty();
                              $(str).appendTo("#sharedobject_list");
////////////////////////////////////////////////////////////////////////////////////////////////////////

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

    });

  </script>
</head>
<body>



<div id="title_bar" class="title_bar_style">

    <label for="second_level_service_name">【发布&nbsp;step2】&nbsp&nbsp服务名:</label>
    <label id="second_level_service_name"></label>
    ,&nbsp;

    <label for="first_level_service_name"  >上一级服务名:</label>
    <label id="first_level_service_name"  ></label>
    ,&nbsp;

    <label for="plan_id"  >plan ID:</label>
    <label id="plan_id"  >C++</label>

</div>
<!--------------------------------------------------------------------------------->


<!--------------------------------------------------------------------------------->
    <div class="form_style">
    <button type="button" class="btn-small"  style="border-width: 0px;width:20px" id="btn_toggle_config" onclick="onConfigToggleBtnClicked()">-</button>
    <label for="div_config"  >◆◆配置文件:</label>
    <div  class="form_style" id="div_config" style="margin-left: 10px;margin-right: 10px;">

        <div>
            <label for="div_config"  >配置文件版本列表:</label>
            <select id="config_version_list" size="8"  class="form-control" ondblclick="onTagListDbClicked()" >
            </select>
            <button type="button" class="btn-small" id="btn_del_config_version" onclick="onConfigTagDelBtnClicked()">delete selected</button>
        </div>
        <br>
        <!--<div   style="width:50%;float:left; margin-left: 5px;">-->
        <div>
            <label for="config_content"  >配置文件内容:</label> <br>
            <textarea class="form-control" id="config_content" rows="12" style="resize:none;"></textarea>

            新tag：<input type="text" style="width:150px;display: inline-block" id="new_tag_name">
            备注：<input type="text" style="width:250px;display: inline-block" id="new_tag_memo">

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
            <label for="IDL_list"  >文件版本列表:</label>
            <select id="IDL_list" size="5"  class="form-control" ondblclick="onIDLListDbClicked()" >
            </select>
            <button type="button" class="btn-small" id="btn_del_IDL" onclick="onIDLDelBtnClicked()">delete selected</button>
        </div>
        <br>

        <div>
            <label for="IDL_content"  >IDL文件内容:</label> <br>
            <textarea class="form-control" id="IDL_content" rows="17" style="resize:none;"></textarea>

            新tag：<input type="text" style="width:150px;display: inline-block" id="new_IDL_name">
            备注：<input type="text" style="width:250px;display: inline-block" id="new_IDL_memo">
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
           <form action="/LibrarayFileUpload" method="post"    id="library_upload_form"  enctype="multipart/form-data">
            新代码库/资源文件：<input style="width:250px;display: inline-block"  type="file" name="new_library_file_name" size="150" id="new_library_file_name" />
            备注：<input type="text" style="width:250px;display: inline-block" id="new_library_memo" name="new_library_memo"><br>
               <input type="hidden" name="first_level_service_name_of_new_library" id="first_level_service_name_of_new_library">
               <input type="hidden" name="second_level_service_name_of_new_library" id="second_level_service_name_of_new_library">
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
            <form action="/SharedobjectUpload" method="post"    id="sharedobject_upload_form"  enctype="multipart/form-data">
                新版本插件：<input style="width:250px;display: inline-block"  type="file" name="new_sharedobject_name" size="150" id="new_sharedobject_name" /><br>
                tag：<input type="text" style="width:250px;display: inline-block" id="new_sharedobject_tag" name="new_sharedobject_tag"><br>
                备注：<input type="text" style="width:250px;display: inline-block" id="new_sharedobject_memo" name="new_sharedobject_memo"><br>
                <input type="hidden" name="first_level_service_name_of_new_sharedobject"  id="first_level_service_name_of_new_sharedobject">
                <input type="hidden" name="second_level_service_name_of_new_sharedobject" id="second_level_service_name_of_new_sharedobject">
                <button type="submit" style="display:inline-block"  id="btn_new_sharedobject" >upload file</button>
            </form>
        </div>
        <div class="clear"></div>
    </div>
</div>
<!--------------------------------------------------------------------------------->
        <div id="result_message"  class="result_msg_style">
        </div>

</div>


</body>
</html>
