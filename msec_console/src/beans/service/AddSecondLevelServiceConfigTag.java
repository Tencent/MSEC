
/**
 * Tencent is pleased to support the open source community by making MSEC available.
 *
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the GNU General Public License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. You may 
 * obtain a copy of the License at
 *
 *     https://opensource.org/licenses/GPL-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the 
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */


package beans.service;

import beans.dbaccess.SecondLevelService;
import beans.dbaccess.SecondLevelServiceConfigTag;
import beans.request.AddSecondLevelServiceConfigTagRequest;
import beans.response.AddSecondLevelServiceConfigTagResponse;
import beans.response.AddSecondLevelServiceConfigTagResponse;
import ngse.org.DBUtil;
import ngse.org.JsonRPCHandler;
import org.apache.log4j.Logger;
import org.ini4j.Ini;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

/**
 * Created by Administrator on 2016/1/28.
 * 更新标准服务的配置信息，新增一个版本
 */
public class AddSecondLevelServiceConfigTag extends JsonRPCHandler {
    private boolean SaveConfigContent(String filename, String content)
    {
        File file = new File(filename);
        file.delete();
        try {
            file.createNewFile();
            FileOutputStream out = new FileOutputStream(file);
            out.write(content.getBytes(Charset.forName("UTF-8")));

            out.close();
            return true;
        }catch (IOException e)
        {
            e.printStackTrace();
            return false;
        }

    }
    private int getStandardServicePort(DBUtil util, String flsn, String slsn) throws Exception
    {
        Logger logger = Logger.getLogger(AddSecondLevelServiceConfigTag.class);

        String sql = "select port from t_second_level_service where first_level_service_name=? " +
                "and second_level_service_name=? and type='standard'";
        List<Object> params = new ArrayList<>();
        params.add(flsn);
        params.add(slsn);

        List<SecondLevelService> svcList = util.findMoreRefResult(sql, params, SecondLevelService.class);
        if (svcList == null || svcList.size() != 1) {
            throw new Exception("invalid record in t_second_level_service:" + flsn + "." + slsn);
        }
        return svcList.get(0).getPort();

    }
    public  void addPortConfig(String filename, int port) throws Exception
    {

        final String  SECTION_NAME = "SRPC";
        final String  KEY = "listen";
        //listen=e123/udp 234/tcp
        final String VALUE = String.format("%d/udp %d/tcp", port, port);
        Ini ini = new Ini();
        ini.load(new File(filename));
        Ini.Section section = ini.get(SECTION_NAME);
        boolean bChanged = false;
        if (section == null)
        {
            section = ini.add(SECTION_NAME);
            section.add(KEY, VALUE);
            bChanged = true;
        }
        else
        {
            String valueStr = section.get(KEY);
            if (valueStr == null)
            {
                section.add(KEY, VALUE);
                bChanged = true;
            }
            else
            {
                if (!valueStr.equalsIgnoreCase(VALUE))
                {
                    //logger.error("port config exists,but value mismatch!"+valueStr+"!="+VALUE);
                    section.put(KEY, VALUE);
                    bChanged = true;
                }
            }
        }
        if (bChanged)
        {
            ini.store(new File(filename));
        }



    }
    public AddSecondLevelServiceConfigTagResponse exec(AddSecondLevelServiceConfigTagRequest request)
    {
        AddSecondLevelServiceConfigTagResponse response = new AddSecondLevelServiceConfigTagResponse();
        response.setMessage("unkown error.");
        response.setStatus(100);
        String result = checkIdentity();
        if (!result.equals("success"))
        {
            response.setStatus(99);
            response.setMessage(result);
            return response;
        }
        if (request.getTag_name() == null ||
                request.getTag_name().equals("") ||
                request.getFirst_level_service_name() == null || request.getFirst_level_service_name().length()<1||
                request.getSecond_level_service_name() == null || request.getSecond_level_service_name().length()<1||
                request.getContent() == null || request.getContent().length() < 1)

        {
            response.setMessage("Some request field is  empty.");
            response.setStatus(100);
            return response;
        }

        DBUtil util = new DBUtil();
        if (util.getConnection() == null)
        {
            response.setMessage("DB connect failed.");
            response.setStatus(100);
            return response;
        }



        try {
            //保存文件内容到服务器磁盘
            String filename = SecondLevelServiceConfigTag.getConfigFileName(request.getFirst_level_service_name(),
                    request.getSecond_level_service_name(), request.getTag_name());
            if (!SaveConfigContent(filename, request.getContent()) )
            {
                response.setMessage("save file failed.");
                response.setStatus(100);
                return response;
            }
            //写入端口配置信息
            int port = getStandardServicePort(util, request.getFirst_level_service_name(),
                                    request.getSecond_level_service_name());

            addPortConfig(filename, port);


            //插入数据库
            String sql;
            List<Object> params = new ArrayList<Object>();

            sql = "insert into t_config_tag(tag_name, memo, second_level_service_name,first_level_service_name) values(?,?,?,?)";
            params.add(request.getTag_name());
            if (request.getMemo() == null ){params.add("");}else{params.add(request.getMemo());}
            params.add(request.getSecond_level_service_name());
            params.add(request.getFirst_level_service_name());
            int addNum = util.updateByPreparedStatement(sql, params);

            if (addNum > 0)
            {
                response.setAddNumber(addNum);
                response.setMessage("success");
                response.setStatus(0);
                return response;
            }
            else
            {
                response.setAddNumber(addNum);
                response.setMessage("failed to insert table");
                response.setStatus(100);
                return response;
            }
        }
        catch (Exception e)
        {
            response.setMessage("add record failed:"+e.toString());
            response.setStatus(100);
            e.printStackTrace();
            return response;
        }
        finally {
            util.releaseConn();
        }


    }
}
