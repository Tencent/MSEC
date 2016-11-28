
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



package org.ngse.sink.mysql;

import com.google.common.base.Preconditions;
import com.google.common.base.Throwables;
import com.google.common.collect.Lists;
import org.apache.flume.*;
import org.apache.flume.conf.Configurable;
import org.apache.flume.sink.AbstractSink;
import org.apache.flume.formatter.output.PathManager;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.sql.Timestamp;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;
import java.lang.String;
import java.lang.StringBuilder;
import java.sql.*;
import java.util.Map;
import java.util.Date;
import java.util.List;
import java.util.ArrayList;
import java.text.SimpleDateFormat;

import com.google.common.util.concurrent.ThreadFactoryBuilder;

public class MysqlSink extends AbstractSink implements Configurable {

    private Logger LOG = LoggerFactory.getLogger(MysqlSink.class);

    //members for mysql
    private String hostname;
    private String port;
    private String databaseName;
    private String tablePrefix;
    private String tableName;
    private String user;
    private String password;
    private PreparedStatement preparedStatement;
    private Connection conn;
    private int batchSize;
    private List<String> columnList;
    private String  columnListStr;
	private String  createTableSqlFile;

    //members for file-autoload
    private File directory;
    private static final long defaultRollInterval = 60;
    private long rollInterval;
    private int maxContentLength;
    private OutputStream outputStream;
    private ScheduledExecutorService rollService;
    private PathManager pathController;
    private volatile boolean shouldRotate;

    public MysqlSink() {
        LOG.info("MysqlSink constructed...");
        pathController = new PathManager();
        shouldRotate = false;
    }

    @Override
    public void configure(Context context) {
        hostname = context.getString("hostname");
        Preconditions.checkNotNull(hostname, "hostname must be set!!");
        port = context.getString("port");
        Preconditions.checkNotNull(port, "port must be set!!");
        databaseName = context.getString("databaseName");
        Preconditions.checkNotNull(databaseName, "databaseName must be set!!");
        tablePrefix = context.getString("tableprefix");
        Preconditions.checkNotNull(tablePrefix, "tableprefix must be set!!");
        user = context.getString("user");
        Preconditions.checkNotNull(user, "user must be set!!");
        password = context.getString("password");
        if (password == null) password = "";
        //Preconditions.checkNotNull(password, "password must be set!!");
        batchSize = context.getInteger("batchSize", 100);
        Preconditions.checkNotNull(batchSize > 0, "batchSize must be a positive number!!");

        maxContentLength = context.getInteger("maxContentLength", 1000);
        Preconditions.checkNotNull(maxContentLength > 0, "maxContentLength must be a positive number!!");

        String directory = context.getString("directory");
        String rollInterval = context.getString("loadInterval");
		createTableSqlFile = context.getString("create_table_sql");

        Preconditions.checkArgument(directory != null, "Directory may not be null");
		Preconditions.checkNotNull(createTableSqlFile, "create table sql must be set!!");
        if (rollInterval == null) {
            this.rollInterval = defaultRollInterval;
        } else {
            this.rollInterval = Long.parseLong(rollInterval);
        }

        Preconditions.checkArgument(this.rollInterval > 0, "rollInterval must be positive");
        this.directory = new File(directory);
        tableName = "";
    }

    @Override
    public void start() {
        super.start();
        try {
            //调用Class.forName()方法加载驱动程序
            Class.forName("com.mysql.jdbc.Driver");
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        }

        String url = "jdbc:mysql://" + hostname + ":" + port + "/" + databaseName + "?autoReconnect=true"; 
        //调用DriverManager对象的getConnection()方法，获得一个Connection对象

        try {
            conn = DriverManager.getConnection(url, user, password);
            conn.setAutoCommit(false);

        } catch (SQLException e) {
            e.printStackTrace();
            System.exit(1);
        }

        pathController.setBaseDirectory(directory);
        rollService = Executors.newScheduledThreadPool(
                1,
                new ThreadFactoryBuilder().setNameFormat("MysqlSink-roller-" + Thread.currentThread().getId() + "-%d").build());

      /*
       * Every N seconds, mark that it's time to rotate. We purposefully do NOT
       * touch anything other than the indicator flag to avoid error handling
       * issues (e.g. IO exceptions occuring in two different threads.
       * Resist the urge to actually perform rotation in a separate thread!
       */
        rollService.scheduleAtFixedRate(new Runnable() {
            @Override
            public void run() {
                LOG.debug("Marking time to rotate file {}",
                        pathController.getCurrentFile());
                shouldRotate = true;
            }

        }, rollInterval, rollInterval, TimeUnit.SECONDS);

        LOG.info("MysqlSink {} started.", getName());
    }

    @Override
    public void stop() {
        super.stop();
        if (preparedStatement != null) {
            try {
                preparedStatement.close();
            } catch (SQLException e) {
                e.printStackTrace();
            }
        }

        if (conn != null) {
            try {
                conn.close();
            } catch (SQLException e) {
                e.printStackTrace();
            }
        }
    }

    private int createTable() throws IOException, SQLException
    {
        SimpleDateFormat formatter = new SimpleDateFormat("MMdd");
        String curTableName = "t_dailylog_" + formatter.format(new Date());

        if (curTableName.compareTo(tableName) == 0) {
            return 0;
        }

        DatabaseMetaData md = conn.getMetaData();
        ResultSet rs = md.getTables(null, null, curTableName, null);
        if (rs.next()) {
            tableName = curTableName;
            return 1;
        }

        //create table
        File file = new File(createTableSqlFile);
        BufferedReader bf = new BufferedReader(new FileReader(file));
        StringBuilder sb = new StringBuilder();
        String line = "";

        while (line != null) {
            line = bf.readLine();
            if (line == null)   break;

            sb.append(line);
        }

        String createTableSql = sb.toString().replace("$TABLENAME$", curTableName);
        Statement stmt = conn.createStatement();
        stmt.executeUpdate(createTableSql);
        LOG.info("Create table " + curTableName + " using sql statement: " +createTableSql);
        tableName = curTableName;
        return 2;
    }

    private void getColumnList() throws SQLException
    {
        DatabaseMetaData md = conn.getMetaData();
        ResultSet rs = md.getColumns(null, null, tableName, null);

        columnList = new ArrayList<String>();
        columnListStr = "(";
        while (rs.next()) {
            columnList.add(rs.getString("COLUMN_NAME"));
            columnListStr += rs.getString("COLUMN_NAME");
            if (!rs.isLast()) {
                columnListStr += ",";
            }
        }
        columnListStr += ")";
    }

    private void doSerialize(Event event) throws  IOException
    {
        Map<String, String>  headers;
        String content = "";

        headers = event.getHeaders();

        if (outputStream == null)
        {
            LOG.error("Null stream for Mysql sink serialization!");
            return;
        }

        for (String column: columnList)
        {
            if (column.compareTo("content") == 0) {
                continue;
            }

            if (headers.containsKey(column)) {
                outputStream.write((headers.get(column) + "\t").getBytes());
                headers.remove(column);
            } else if (column.compareTo("instime") == 0){
                Timestamp  timestamp = new Timestamp(new Date().getTime());
                outputStream.write((timestamp.toString() + "\t").getBytes());
            } else {
                outputStream.write("\t".getBytes());  //default value
            }
        }

        for (String headerKey: headers.keySet()) {
            String headerValue = headers.get(headerKey);
            if (headerValue != null && !headerValue.isEmpty()) {
                content += headerKey + "=" + headerValue + " ";
            }
        }

        content += new String(event.getBody());
        content = content.replace('\t', ' ').replace('\n', ' ');
        if (content.length() > maxContentLength) {
            content = content.substring(0, maxContentLength - 15) + "<..truncated..>";
        }
        outputStream.write((content + "\n").getBytes());

        LOG.info("Mysql sink process: " + content + " " + headers.get("IP") + " " + headers.get("Level") + " " +headers.get("RPCName"));
    }

    @Override
    public Status process() throws EventDeliveryException {
        Status result = Status.READY;
        Channel channel = getChannel();
        Transaction transaction = channel.getTransaction();
        Event event;

        String loadSql;
        Statement stmt;

        //Create table in database
        try {
            if (createTable() > 0) {
                getColumnList();
            }
        }
        catch (Exception e) {
            LOG.error("Failed to create table!" + e);
            e.printStackTrace();
            System.exit(1);

            result = Status.BACKOFF;
            return result;
        }

        if (shouldRotate) {
            LOG.debug("Time to rotate {}", pathController.getCurrentFile());

            if (outputStream != null) {
                LOG.debug("Closing file {}", pathController.getCurrentFile());

                try {
                    outputStream.flush();
                    outputStream.close();
                    shouldRotate = false;
                } catch (IOException e) {
                    throw new EventDeliveryException("Unable to rotate file "
                            + pathController.getCurrentFile() + " while delivering event", e);
                } finally {
                    outputStream = null;
                }

                //Load the closed file info mysql
                try {
                    loadSql = "load data infile \"" + pathController.getCurrentFile().getCanonicalPath() + "\" into table " + tableName + " fields terminated by '\t' " +columnListStr;
                    LOG.info("load data sql: " + loadSql);

                    stmt = conn.createStatement();
                    stmt.executeUpdate(loadSql);
                    conn.commit();
                }
                catch (Exception e) {
                    LOG.error("Failed to load data info mysql! " + e);
                    e.printStackTrace();
                }

                //rotate to next file
                pathController.rotate();
            }
        }

        try {
            transaction.begin();
            for (int i = 0; i < batchSize; i++) {
                event = channel.take();
                if (event != null) {
                    //Serialize the event to the file!
                    //Reopen the output file
                    if (outputStream == null) {
                        File currentFile = pathController.getCurrentFile();
                        LOG.debug("Opening output stream for file {}", currentFile);
                        try {
                            outputStream = new BufferedOutputStream(
                                    new FileOutputStream(currentFile));
                        } catch (IOException e) {
                            throw new EventDeliveryException("Failed to open file "
                                    + pathController.getCurrentFile() + " while delivering event", e);
                        }
                    }

                    doSerialize(event);
                } else {
                    // No events found, request back-off semantics from runner
                    result = Status.BACKOFF;
                    break;
                }
            }

            if (outputStream != null)   {
                outputStream.flush();
            }
            transaction.commit();
        } catch (Exception ex) {
            transaction.rollback();
            throw new EventDeliveryException("Failed to process transaction", ex);
        } finally {
            transaction.close();
        }

        return result;
    }
}


