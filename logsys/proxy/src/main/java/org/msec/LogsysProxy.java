
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

import org.apache.commons.cli.*;
import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;

import java.io.*;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Properties;

public class LogsysProxy {

    static Logger logger = Logger.getLogger(LogsysProxy.class.getName());

    public static void main(String[] args) {

        try {
            Options options = new Options();
            Option option = new Option("c", "conf", true, "configuration filename");
            option.setRequired(true);
            options.addOption(option);

            option = new Option("h", "help", false, "show help");
            options.addOption(option);

            CommandLineParser parser = new GnuParser();
            CommandLine commandLine = null;

            try {
                commandLine = parser.parse(options, args);
            } catch (ParseException e) {
                System.out.println("Parse command line failed.");
                e.printStackTrace();
                return;
            }

            if (commandLine.hasOption('h')) {
                new HelpFormatter().printHelp("LogsysProxy", options, true);
                return;
            }

            //读取配置
            String conFilename = commandLine.getOptionValue('c');
            String userdir = System.getProperty("user.dir");
            Properties props = new Properties();
            InputStream in = new FileInputStream(conFilename);
            props.load(in);
            in.close();

            PropertyConfigurator.configure("conf/log4j.properties");
            Class.forName("com.mysql.jdbc.Driver");

            //默认端口: 30150
            int listenPort = Integer.parseInt(props.getProperty("listen", "30150"));
            ServerSocket serverSocket = new ServerSocket(listenPort);
            Socket socket = null;

            logger.info("Logsys Proxy Started.");
            //循环监听等待客户端的链接, 每个请求创建一个线程来处理
            while (true) {
                socket = serverSocket.accept();
                
                InetAddress address = socket.getInetAddress();
                logger.info("Request coming ... " + address.getHostAddress());
                ServerThread serverThread = new ServerThread(socket, conFilename, props);
                serverThread.start();          
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch(ClassNotFoundException e) {
            e.printStackTrace();
        }
    }
}