
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


package ngse.remote_shell;
import java.io.*;
import java.net.*;
import java.security.*;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.*;
import java.util.logging.*;

class MyLogHander extends Formatter {
    @Override
    public String format(LogRecord record) {
        return record.getLevel() + ":" + record.getMessage()+"\n";
    }
}

public class Main {

    private  static void initLog()
    {
        Logger logger = Logger.getLogger("remote_shell");
        logger.setLevel(Level.ALL);
        try {
            FileHandler handler = new FileHandler("./server%g.log");
            handler.setFormatter(new MyLogHander());
            logger.addHandler(handler);
        }catch (IOException e)
        {System.out.println("logger init failed!");}
    }
    static public PrivateKey privateKey = null;







    public static void main(String[] args){

        if (args.length >= 1 && args[0].equals("newRSAKey"))
        {
            Tools.newRSAKeyAndSave();
            return;
        }


        System.out.println("the server of remote shell start.");

        initLog();

        ExecutorService service = Executors.newFixedThreadPool(100);

        int port = 9981;
        if (args.length >= 1)//端口
        {
            port= Integer.valueOf(args[0]).intValue();
        }
        if (args.length >= 2)
        {
            // 如果指定了私钥文件，就加载起来
            // 发送指令文件让远程agent执行的时候，用该私钥签名，避免恶意冒充server给agent下发指令
           if ( (privateKey = Tools.loadPrivKeyFromFile(args[1])) != null)
           {
               System.out.println("load private key  successfully");
           }
        }



        ServerSocket socket = null;
        try {
            socket = new ServerSocket(port);
            //socket.setSoTimeout(10);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.exit(-1);
        }

        while (true)
        {
            Socket newsock = null;
            try {
                 newsock = socket.accept();//接收来自客户端或者agent的连接
                 newsock.setSoTimeout(30000);
            }
            catch (IOException e)
            {
                e.printStackTrace();
                System.exit(-1);
            }
            //交给线程池处理该连接的请求，短链接
            service.execute(new Worker(newsock));
        }



    }
}
