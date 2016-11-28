
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


package org.msec;

import net.sf.json.JSONException;
import org.apache.commons.lang.StringUtils;
import org.apache.log4j.Logger;
import org.codehaus.jackson.map.ObjectMapper;

import java.io.FileOutputStream;
import java.io.IOException;
import java.sql.*;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.*;
import java.util.Date;

public class LogQuery {

    static Logger logger = Logger.getLogger(LogQuery.class.getName());
    static final String tableNamePrefix = "t_dailylog_";
    static final String timeColumnName = "unix_timestamp(instime)";
    static SimpleDateFormat dateFormatter;
    static SimpleDateFormat dayFormatter;
    static int columnCount;
    static final String seperator = ", ";

    static {
        dateFormatter = new SimpleDateFormat("yyyy-MM-dd");
        dayFormatter = new SimpleDateFormat("MMdd");
        columnCount = 0;
    }

    Connection conn;
    Statement stmt;
    private String mysqlHost;
    private int mysqlPort;
    private String mysqlUser;
    private String mysqlPasswd;
    private List<String>   builtinColumns;
    private List<String>   delFields;
    private ArrayList<Integer>  delColumns;

    LogQuery(String host, int port, String user, String passwd, String delFieldsConf) throws ClassNotFoundException, SQLException {
        mysqlHost = host;
        mysqlPort = port;
        mysqlUser = user;
        mysqlPasswd = passwd;

        String url = "jdbc:mysql://" + mysqlHost + ":" + mysqlPort + "/logsys?autoReconnect=true";
        conn = DriverManager.getConnection(url, mysqlUser, mysqlPasswd);  //连接数据库
        stmt = conn.createStatement(ResultSet.TYPE_SCROLL_INSENSITIVE,ResultSet.CONCUR_READ_ONLY); //创建Statement对象
        logger.info("Connect to db OK！");

        builtinColumns = new ArrayList<String>();
        builtinColumns.add("reqid");
        builtinColumns.add("ip");
        builtinColumns.add("clientip");
        builtinColumns.add("serverip");
        builtinColumns.add("level");
        builtinColumns.add("rpcname");
        builtinColumns.add("fileline");
        builtinColumns.add("function");
        builtinColumns.add("instime");
        builtinColumns.add("content");

        if (delFieldsConf != null) {
            delFields = new ArrayList<String>(Arrays.asList(delFieldsConf.split(",")));
        } else {
            delFields = new ArrayList<String>();
        }
    }

    private List<LogField> getColumnList() throws SQLException
    {
        String tableName =  "t_dailylog_" + dayFormatter.format(new Date());
        DatabaseMetaData md = conn.getMetaData();
        ResultSet rs = md.getColumns(null, null, tableName, null);

        List<LogField> columnList = new ArrayList<LogField>();
        String columnName;
        String columnType;
        String transColumnType = "";
        while (rs.next()) {
            columnName = rs.getString("COLUMN_NAME");
            columnType = rs.getString("TYPE_NAME").toLowerCase();

            if (builtinColumns.contains(columnName.toLowerCase())) {
                continue;
            }

            //过滤掉已删除字段
            if (delFields.contains(columnName.toLowerCase())) {
                continue;
            }
            if (columnType.contains("int")) {
                transColumnType = "Integer";
            } else if (columnType.contains("char") || columnType.contains("text")) {
                transColumnType = "String";
            } else if (columnType.contains("date") || columnType.contains("time")) {
                transColumnType = "Date";
            } else {
                transColumnType = columnType;
            }
            columnList.add(new LogField(columnName, transColumnType, ""));
        }
        return columnList;
    }

    public List<String> getTableList(String startDateStr, String endDateStr) throws ParseException, SQLException {
        Date startDate = dateFormatter.parse(startDateStr);
        Date endDate = dateFormatter.parse(endDateStr);
        List<String> ret = new ArrayList<String>();

        Calendar start = Calendar.getInstance();
        start.setTime(startDate);
        Calendar end = Calendar.getInstance();
        end.setTime(endDate);

        for (Date date = start.getTime(); !start.after(end); start.add(Calendar.DATE, 1), date = start.getTime()) {
            //判断表是否存在
            String tableName = tableNamePrefix + dayFormatter.format(date);
            DatabaseMetaData md = conn.getMetaData();
            ResultSet rs = md.getTables(null, null, tableName, null);
            if (rs.next()) {
                ret.add(tableNamePrefix + dayFormatter.format(date));
            }
        }
        return ret;
    }

    public void close() throws SQLException {
        if (stmt != null) {
            stmt.close();
        }
        if (conn != null) {
            conn.close();
        }
    }

    public org.msec.LogsysRsp.ModifyFieldsRsp modifyFields(LogsysReq.ModifyFieldsReq  req, String confFilename, Properties props)
    {
        org.msec.LogsysRsp.ModifyFieldsRsp rsp = new org.msec.LogsysRsp.ModifyFieldsRsp();

        String sql;
        String tableName =  "t_dailylog_" + dayFormatter.format(new Date());
        String addField;

        if (0 != req.operator.compareToIgnoreCase("ADD") &&
                0 != req.operator.compareToIgnoreCase("DEL")) {
            rsp.setRet(-1);
            rsp.setErrmsg("Unknown Operator.");
            return rsp;
        }

        List<LogField> list;  //检查是否在已有字段中
        boolean  found = false;
        try {
            list = getColumnList();
            //检查是否在已有字段中
            for (int i=0; i<list.size(); ++i) {
                if (0 == list.get(i).field_name.compareToIgnoreCase(req.getFieldName())) {
                    found = true;
                    break;
                }
            }
        } catch (SQLException e) {
            rsp.setRet(-2);
            rsp.setErrmsg(e.getMessage());
            return rsp;
        }

        System.out.println("Check field exist: " + found);
        if (0 == req.operator.compareToIgnoreCase("ADD")) {
            if (found) {
                rsp.setRet(-3);
                rsp.setErrmsg("Add Duplicate Field Name!");
                return rsp;
            }

            //曾经删除过的字段
            if (delFields.contains(req.getFieldName().toLowerCase())) {
                rsp.setRet(-4);
                rsp.setErrmsg("Add Duplicate Field Name!");
                return rsp;
            }

            if (0 == req.getFieldType().compareToIgnoreCase("String")) {
                addField = req.getFieldName() + " varchar(1024) ";
            } else if (0 == req.getFieldType().compareToIgnoreCase("Integer")) {
                addField = req.getFieldName() + " int ";
            } else {
                rsp.setRet(-5);
                rsp.setErrmsg("Unknown Field Type.");
                return rsp;
            }

            sql = "ALTER TABLE " + tableName + " add " + addField;
            System.out.println("Add column sql: " + sql);
            try {
                stmt.executeUpdate(sql);
                rsp.setRet(0);
                rsp.setErrmsg("Succeed.");
            } catch (SQLException e) {
                rsp.setRet(-6);
                rsp.setErrmsg(e.getMessage());
            }
            System.out.println("Add column finished. ");

            columnCount = 0; //清空columncount, 重新获取
        }

        if (0 == req.operator.compareToIgnoreCase("DEL")) {
            if (!found) {
                rsp.setRet(-10);
                rsp.setErrmsg("Delete Field Not Exist!");
                return rsp;
            }
            delFields.add(req.getFieldName().toLowerCase());
            props.setProperty("delfields" , StringUtils.join(delFields, ","));

            //写配置文件
            try {
                FileOutputStream fos = null;
                fos = new FileOutputStream(confFilename);
                props.store(fos, "Copy Right");
                fos.close();

                rsp.setRet(0);
                rsp.setErrmsg("Succeed.");
            } catch (IOException e) {
                e.printStackTrace();

                rsp.setRet(-11);
                rsp.setErrmsg("Delete Fields Failed.");
            }

        }
        return rsp;
    }

    public org.msec.LogsysRsp.GetFieldsRsp getFields(LogsysReq.GetFieldsReq  req)
    {
        List<LogField>  list = null;
        try {
            list = getColumnList();
        } catch (SQLException e) {
            e.printStackTrace();
        }
        org.msec.LogsysRsp.GetFieldsRsp rsp = new org.msec.LogsysRsp.GetFieldsRsp();
        rsp.setRet(0);
        rsp.setErrmsg("Succeed.");
        rsp.setFieldsInfo(list);
        return rsp;
    }

    public org.msec.LogsysRsp.QueryLogRsp queryRecords(int logLevel, Map<String, String> headerFilter, int maxRetNum,
                            String startDate, String endDate,
                            String startTime, String endTime, String whereCondition) {
        long timeStart = System.currentTimeMillis();
        String where = "";
        String querySql = "";
        Iterator<Map.Entry<String, String>> entries = headerFilter.entrySet().iterator();
        Map.Entry<String, String> entry;
        while (entries.hasNext()) {
            if (!where.isEmpty()) {
                where += " and ";
            } else {
                where += " where ";
            }
            entry = entries.next();
            where += entry.getKey() + " = \'" + entry.getValue() + "\'";
        }

        if (whereCondition != null && !whereCondition.isEmpty()) {
            if (!where.isEmpty())   {
                where += " and " + whereCondition;
            }
            else {
                where += " where " + whereCondition;
            }
        }

        List<String> tableList = null;
        String sql;
        List<String>  logsHeads = new ArrayList<String>();
        List<Object>  logRecords = new ArrayList<Object>();

        int totalRetNum = 0;
        int currentCnt = 0;
        int ret = 0;
        org.msec.LogsysRsp.QueryLogRsp rsp = new org.msec.LogsysRsp.QueryLogRsp();
        rsp.setRet(0);
        rsp.setErrmsg("Succeed.");

        try {
            tableList = getTableList(startDate, endDate);

            logger.info("Timecost for query process phase1: " + (System.currentTimeMillis() - timeStart) + "ms");
            timeStart = System.currentTimeMillis();

			for (int tableIndex = 0; tableIndex < tableList.size(); tableIndex++) {
                sql = "select * from " + tableList.get(tableIndex) + where;
                if (tableIndex == 0) {
                    if (sql.contains("where")) {
                        sql += " and ";
                    } else {
                        sql += " where ";
                    }
                    sql += timeColumnName + " >= unix_timestamp(\"" + startDate + " " + startTime + "\") ";
                }
                if (tableIndex == tableList.size() - 1) {
                    if (sql.contains("where")) {
                        sql += " and ";
                    } else {
                        sql += " where ";
                    }
                    sql += timeColumnName + " <= unix_timestamp(\"" + endDate + " " + endTime + "\") ";
                }

                sql += " limit 0," + (maxRetNum - totalRetNum);
                logger.info("query sql: " + sql);
                querySql += sql + "\n";

                ResultSet rs = stmt.executeQuery(sql);//创建数据对象
                logger.info("Timecost for query process table(" + tableList.get(tableIndex) +"): " + (System.currentTimeMillis() - timeStart) + "ms");
                timeStart = System.currentTimeMillis();

                rs.last();
                int rowCount = rs.getRow();
                rs.beforeFirst();

                if (logsHeads.size() == 0 || columnCount == 0) {
                    ResultSetMetaData rsmd = rs.getMetaData();
                    logsHeads.clear();
                    columnCount = rsmd.getColumnCount();
                    delColumns = new ArrayList<Integer>();

                    for (int i = 0; i < columnCount; i++) {
                        if (delFields.contains(rsmd.getColumnName(i + 1).toLowerCase())) {
                            delColumns.add(Integer.valueOf(i));
                            continue;
                        }

                        logsHeads.add(rsmd.getColumnName(i + 1));
                    }
                }

                while (rs.next()) {
                    currentCnt++;

                    List<String>  logOneRecord = new ArrayList<String>();
                    for (int i=0; i<columnCount; i++) {
                        if (delColumns.contains(Integer.valueOf(i))) {
                            continue;
                        }
                        //logJsonEntry.a
                        if (rs.getObject(i+1) == null)
                            logOneRecord.add("");
                        else
                            logOneRecord.add(rs.getObject(i+1).toString());
                    }
                    logRecords.add(logOneRecord);
                }
                rs.close();

                //日志结果存入JSON
                totalRetNum += rowCount;
                if (totalRetNum >= maxRetNum)   break;
            }

        } catch (ParseException e) {
            e.printStackTrace();
            rsp.setRet(-1);
            rsp.setErrmsg(e.toString());
        } catch (SQLException e) {
            e.printStackTrace();
            rsp.setRet(-2);
            rsp.setErrmsg(e.toString());
        } catch (JSONException e) {
            e.printStackTrace();
            rsp.setRet(-3);
            rsp.setErrmsg(e.toString());
        }

        try {
            rsp.setRet(0);
            rsp.setLines(totalRetNum);
            rsp.setQuerySql(querySql);
            rsp.setHeads(logsHeads);
            rsp.setRecords(logRecords);
        } catch (JSONException e) {
            e.printStackTrace();
            rsp.setRet(-4);
            rsp.setErrmsg("json write error.");
        }
        return rsp;
    }

    public void showRecords() throws SQLException {
        String tableName = tableNamePrefix + dayFormatter.format(new Date());
        DatabaseMetaData md = conn.getMetaData();
        ResultSet rs = md.getTables(null, null, tableName, null);
        if (rs.next()) {
            System.out.println("table " + tableName + " exist");
        } else {
            System.out.println("table " + tableName + " not exist");
            return;
        }

        String columnList = "";
        rs = md.getColumns(null, null, tableName, null);
        while (rs.next()) {
            columnList += rs.getString("COLUMN_NAME");
            if (!rs.isLast())
                columnList += ",";
        }
        System.out.println(columnList);

        String sql = "select * from " + tableName;    //要执行的SQL
        rs = stmt.executeQuery(sql);//创建数据对象
        System.out.println("ip" + "\t" + "level" + "\t" + "rpcname" + "\t\t" + "time" + "\t" + "content");
        while (rs.next()) {
            System.out.print(rs.getString(1) + "\t");
            System.out.print(rs.getString(2) + "\t");
            System.out.print(rs.getString(3) + "\t");
            System.out.print(rs.getString(4) + "\t");
            System.out.print(rs.getString(5) + "\t");
            System.out.println();
        }
        rs.close();
    }

    public org.msec.LogsysRsp.CallGraphRsp callGraph(int logLevel, Map<String, String> headerFilter, int maxRetNum,
                                              String startDate, String endDate,
                                              String startTime, String endTime, String whereCondition) {
        long timeStart = System.currentTimeMillis();
        String where = "";
        String querySql = "";
        Iterator<Map.Entry<String, String>> entries = headerFilter.entrySet().iterator();
        Map.Entry<String, String> entry;
        while (entries.hasNext()) {
            if (!where.isEmpty()) {
                where += " and ";
            } else {
                where += " where ";
            }
            entry = entries.next();
            where += entry.getKey() + " = \'" + entry.getValue() + "\'";
        }

        if (whereCondition != null && !whereCondition.isEmpty()) {
            if (!where.isEmpty())   {
                where += " and " + whereCondition;
            }
            else {
                where += " where " + whereCondition;
            }
        }

        List<String> tableList = null;
        String sql;
        List<String>  logsHeads = new ArrayList<String>();
        List<Object>  logRecords = new ArrayList<Object>();
        Set<org.msec.LogsysRsp.CallPair>   callPairSet = new HashSet<org.msec.LogsysRsp.CallPair>(128);

        int totalRetNum = 0;
        int currentCnt = 0;
        int ret = 0;
        org.msec.LogsysRsp.CallGraphRsp  rsp = new org.msec.LogsysRsp.CallGraphRsp();
        rsp.setRet(0);
        rsp.setErrmsg("Succeed.");

        try {
            tableList = getTableList(startDate, endDate);

            logger.info("Timecost for query process phase1: " + (System.currentTimeMillis() - timeStart) + "ms");
            timeStart = System.currentTimeMillis();

            for (int tableIndex = 0; tableIndex < tableList.size(); tableIndex++) {
                sql = "select ServiceName,content,RPCName from " + tableList.get(tableIndex) + where;
                if (tableList.size() == 1) {
                    if (sql.contains("where")) {
                        sql += " and ";
                    } else {
                        sql += " where ";
                    }
                    sql += timeColumnName + " between unix_timestamp(\"" + startDate + " " + startTime + "\") " +
                        " and unix_timestamp(\"" + endDate + " " + endTime + "\") ";
                }
                else {
                    if (tableIndex == 0) {
                        if (sql.contains("where")) {
                            sql += " and ";
                        } else {
                            sql += " where ";
                        }
                        sql += timeColumnName + " >= unix_timestamp(\"" + startDate + " " + startTime + "\") ";
                    }
                    if (tableIndex == tableList.size() - 1) {
                        if (sql.contains("where")) {
                            sql += " and ";
                        } else {
                            sql += " where ";
                        }
                        sql += timeColumnName + " <= unix_timestamp(\"" + endDate + " " + endTime + "\") ";
                    }
                }

                sql += " limit 0," + (maxRetNum - totalRetNum);
                logger.info("query sql: " + sql);
                querySql += sql + "\n";

                ResultSet rs = stmt.executeQuery(sql);//创建数据对象
                logger.info("Timecost for query process table(" + tableList.get(tableIndex) +"): " + (System.currentTimeMillis() - timeStart) + "ms");
                timeStart = System.currentTimeMillis();

                rs.last();
                int rowCount = rs.getRow();
                rs.beforeFirst();

                if (logsHeads.size() == 0 || columnCount == 0) {
                    ResultSetMetaData rsmd = rs.getMetaData();
                    logsHeads.clear();
                    columnCount = rsmd.getColumnCount();
                    delColumns = new ArrayList<Integer>();

                    for (int i = 0; i < columnCount; i++) {
                        if (delFields.contains(rsmd.getColumnName(i + 1).toLowerCase())) {
                            delColumns.add(Integer.valueOf(i));
                            continue;
                        }

                        logsHeads.add(rsmd.getColumnName(i + 1));
                    }
                }

                while (rs.next()) {
                    currentCnt++;

                    String content = rs.getObject(2).toString();

                    System.out.println(rs.getObject(1).toString() + " " + rs.getObject(2).toString());
                    int pos = content.indexOf("Caller=");
                    if (pos < 0) {
                        pos = content.indexOf("caller=");
                    }
                    if (pos >= 0) {
                        pos += 7;
                        int pos2 = content.indexOf(" ", pos);
                        org.msec.LogsysRsp.CallPair callPair = new org.msec.LogsysRsp.CallPair();
                        callPair.setFrom(content.substring(pos, pos2));
                        callPair.setTo(rs.getObject(1).toString());
                        callPair.setRpcname(rs.getObject(3).toString());

                        callPairSet.add(callPair);
                    }
                }
                rs.close();

                //日志结果存入JSON
                totalRetNum += rowCount;
                if (totalRetNum >= maxRetNum)   break;
            }

        } catch (ParseException e) {
            e.printStackTrace();
            rsp.setRet(-1);
            rsp.setErrmsg(e.toString());
        } catch (SQLException e) {
            e.printStackTrace();
            rsp.setRet(-2);
            rsp.setErrmsg(e.toString());
        } catch (JSONException e) {
            e.printStackTrace();
            rsp.setRet(-3);
            rsp.setErrmsg(e.toString());
        }

        try {
            rsp.setRet(0);

            String graph = "digraph calls {\n";
            for (org.msec.LogsysRsp.CallPair callPair : callPairSet) {
                graph += "\"" + callPair.getFrom() + "\" -> \"" + callPair.getTo() + "\"";
                graph += " [ label=\"" + callPair.getRpcname() + "\" ]\n";
            }
            graph += "}\n";
            rsp.setGraph(graph);
        } catch (JSONException e) {
            e.printStackTrace();
            rsp.setRet(-4);
            rsp.setErrmsg("json write error.");
        }
        return rsp;
    }

    public static void main(String[] args) {
        try {
            /*LogQuery  logQuery = new LogQuery("127.0.0.1", 5029, "root", "1986admin", "");
            //logQuery.showRecords();
			Map<String, String>  headerFilters = new HashMap<String, String>();
            headerFilters.put("rpcname", "rpc.test");
			LogsysRsp.QueryLogRsp ret = logQuery.queryRecords(0, headerFilters, 3, "2016-02-25", "2016-02-25", "16:00:00", "23:59:59", "");
            logQuery.close();

            System.out.println(ret);
            */

            org.msec.LogsysRsp rsp0 = new org.msec.LogsysRsp();
            org.msec.LogsysRsp.QueryLogRsp rsp = new org.msec.LogsysRsp.QueryLogRsp();
            rsp.setRet(0);
            rsp.setLines(10);
            rsp.setQuerySql("sql");

            List<String> heads = new ArrayList<String>();
            heads.add("head0");
            heads.add("head1");
            heads.add("head2");
            rsp.setHeads(heads);


            List<Object> records = new ArrayList<Object>();
            List<String> record_item = new ArrayList<String>();
            for (int i=0; i<2; ++i)
            {
                record_item.clear();
                if (i == 0) {
                    record_item.add("fvalue0");
                    record_item.add("fvalue1");
                    record_item.add("fvalue2");
                } else {
                    record_item.add("fvalue0000");
                    record_item.add("fvalue0001");
                    record_item.add("fvalue0002");
                }
                records.add(record_item);
            }
            ObjectMapper objectMapper = new ObjectMapper();
            rsp.setRecords(records);
            System.out.println(objectMapper.writeValueAsString(rsp));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}

