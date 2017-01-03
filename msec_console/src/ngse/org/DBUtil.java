
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


package ngse.org;
import java.lang.reflect.Field;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.util.*;

//该类负责与ngse console的后台数据库打交道
public class DBUtil {
    //用户名
    private static final String USERNAME = "ngser";

    private static final String PASSWORD = "ngser";

    private static final String DRIVER = "com.mysql.jdbc.Driver";

//连接的url，特别注意这里的utf8字符集的指定
 private static final String URL = "jdbc:mysql://localhost:3306/ngse?useUnicode=true&characterEncoding=UTF-8";
    private Connection connection;
    private PreparedStatement pstmt;
    private ResultSet resultSet;
    public DBUtil() {
        try{
            Class.forName(DRIVER);


        }catch(Exception e){
            e.printStackTrace();
        }
    }

   //连接数据库，失败就返回null
    public Connection getConnection(){
        try {

            connection = DriverManager.getConnection(URL, USERNAME, PASSWORD);

        } catch (SQLException e) {
            e.printStackTrace();
            return null;
        }
        return connection;
    }


   //增删改数据库，并返回影响的记录的条数
    public int updateByPreparedStatement(String sql, List<Object>params)throws SQLException{
        boolean flag = false;
        int result = -1;
        pstmt = connection.prepareStatement(sql);
        int index = 1;
        if(params != null && !params.isEmpty()){
            for(int i=0; i<params.size(); i++){
                pstmt.setObject(index++, params.get(i));
            }
        }
        result = pstmt.executeUpdate();
        pstmt.close();
        return result;
    }

    //查询数据库，只返回一条记录，该记录的字段保存在Map里返回，字段名作为key，字段值作为value
    public Map<String, Object> findSimpleResult(String sql, List<Object> params) throws SQLException{
        Map<String, Object> map = new HashMap<String, Object>();
        int index  = 1;
        pstmt = connection.prepareStatement(sql);
        if(params != null && !params.isEmpty()){
            for(int i=0; i<params.size(); i++){
                pstmt.setObject(index++, params.get(i));
            }
        }
        resultSet = pstmt.executeQuery();
        ResultSetMetaData metaData = resultSet.getMetaData();
        int col_len = metaData.getColumnCount();
        if (resultSet.next()){
            for(int i=0; i<col_len; i++ ){
                String cols_name = metaData.getColumnName(i+1);
                Object cols_value = resultSet.getObject(cols_name);
                if(cols_value == null){
                    cols_value = "";
                }
                map.put(cols_name, cols_value);
            }
        }
        resultSet.close();
        pstmt.close();
        return map;
    }

    //查询数据库，返回满足条件的所有记录，每条记录保存一个map，字段名作为key，字段值作为value
    public ArrayList<Map<String, Object>> findModeResult(String sql, List<Object> params) throws SQLException{
        ArrayList<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
        int index = 1;
        pstmt = connection.prepareStatement(sql);
        if(params != null && !params.isEmpty()){
            for(int i = 0; i<params.size(); i++){
                pstmt.setObject(index++, params.get(i));
            }
        }
        resultSet = pstmt.executeQuery();
        ResultSetMetaData metaData = resultSet.getMetaData();
        int cols_len = metaData.getColumnCount();
        while(resultSet.next()){
            Map<String, Object> map = new HashMap<String, Object>();
            for(int i=0; i<cols_len; i++){
                String cols_name = metaData.getColumnName(i+1);
                Object cols_value = resultSet.getObject(cols_name);
                if(cols_value == null){
                    cols_value = "";
                }
                map.put(cols_name, cols_value);
            }
            list.add(map);
        }
        resultSet.close();
        pstmt.close();

        return list;
    }

    //查询数据库，只返回一条记录， 使用java反射机制，将记录映射到java类T
    public <T> T findSimpleRefResult(String sql, List<Object> params,
                                     Class<T> cls )throws Exception{
        T resultObject = null;
        int index = 1;
        pstmt = connection.prepareStatement(sql);
        if(params != null && !params.isEmpty()){
            for(int i = 0; i<params.size(); i++){
                pstmt.setObject(index++, params.get(i));
            }
        }
        resultSet = pstmt.executeQuery();
        ResultSetMetaData metaData  = resultSet.getMetaData();
        int cols_len = metaData.getColumnCount();
        while(resultSet.next()){
            resultObject = cls.newInstance();
            for(int i = 0; i<cols_len; i++){
                String cols_name = metaData.getColumnName(i+1);
                Object cols_value = resultSet.getObject(cols_name);
                if(cols_value == null){
                    cols_value = "";
                }
                Field field = cls.getDeclaredField(cols_name);
                field.setAccessible(true);
                field.set(resultObject, cols_value);
            }
        }
        resultSet.close();
        pstmt.close();
        return resultObject;

    }

    //查询数据库，返回所有满足条件的记录， 使用java反射机制，将记录映射到java类T
    public <T> ArrayList<T> findMoreRefResult(String sql, List<Object> params,
                                         Class<T> cls )throws Exception {
        ArrayList<T> list = new ArrayList<T>();
        int index = 1;
        pstmt = connection.prepareStatement(sql);
        if(params != null && !params.isEmpty()){
            for(int i = 0; i<params.size(); i++){
                pstmt.setObject(index++, params.get(i));
            }
        }
        resultSet = pstmt.executeQuery();
        ResultSetMetaData metaData  = resultSet.getMetaData();
        int cols_len = metaData.getColumnCount();
        while(resultSet.next()){
            T resultObject = cls.newInstance();
            for(int i = 0; i<cols_len; i++){
                String cols_name = metaData.getColumnName(i+1);
                Object cols_value = resultSet.getObject(cols_name);

                if(cols_value == null){
                    cols_value = "";
                }
                Field field = cls.getDeclaredField(cols_name);
                field.setAccessible(true); //��javabean�ķ���Ȩ��
                field.set(resultObject, cols_value);
            }
            list.add(resultObject);
        }
        resultSet.close();
        pstmt.close();
        return list;
    }

    //主动结束和数据库的连接
    public void releaseConn() {
        try {
            if (resultSet != null) {

                resultSet.close();
                resultSet = null;

            }

        } catch (SQLException e) {
            e.printStackTrace();
        }
        try
        {
            if (connection != null) {
                connection.close();
                connection = null;
            }
        }
        catch (SQLException e2)
        {
            e2.printStackTrace();
        }
    }
}
