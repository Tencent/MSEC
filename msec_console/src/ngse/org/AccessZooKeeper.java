
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

import org.apache.zookeeper.*;

import java.util.*;
import java.util.concurrent.CountDownLatch;

/**
 * Created by Administrator on 2016/3/2.
 */
public class AccessZooKeeper {

 //   private CountDownLatch countDownLatch=new CountDownLatch(1);
    private ZooKeeper zk = null;
   // private String connectStr = "127.0.0.1:2181";
   private String connectStr = "localhost:8972";


    public AccessZooKeeper()
    {
        connectZooKeeper();
    }



    private void connectZooKeeper()
    {
        try {

            /*
           zk = new ZooKeeper(connectStr, 1000, new Watcher() {
                @Override
                public void process(WatchedEvent event) {
                    System.out.println("已经触发了" + event.getType() + "事件！");
                    if(event.getState()== Event.KeeperState.SyncConnected){
                        countDownLatch.countDown();
                    }
                }
            });
            countDownLatch.await();
            */
            zk = new ZooKeeper(connectStr, 2000, null);
            int i = 0;
            while (zk.getState() == ZooKeeper.States.CONNECTING && i < 20)
            {
                Thread.sleep(100);
                i++;
            }


        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public boolean isConnected()
    {
        return zk.getState().isConnected();
    }
    public void disconnect()
    {
        try {
            zk.close();
        }
        catch (Exception e) {}
    }
    public String write(String path, byte[] data) throws Exception
    {
        return write(path, data, true);
    }

    public String write(String path, byte[] data, boolean persistent) throws Exception
    {
        if (path.length() < 2) { return "path invalid";}

        if (path.substring(path.length()-1).equals("/")) //以 /结尾的话
        {
            path = path.substring(0, path.length()-1);//去掉末尾的/
        }
        //获得各级父节点的路径
        ArrayList<String> parents = new ArrayList<String>();
        int begin = 1;
        while (true)
        {
            int index = path.indexOf("/", begin);
            if (index < 0)
            {
                break;
            }
            String substr = path.substring(0, index);
            parents.add(substr);
            begin = index+1;

        }
        //逐级创建父节点
        for (int i = 0; i < parents.size(); i++) {
            try {
                zk.create(parents.get(i), null, ZooDefs.Ids.OPEN_ACL_UNSAFE, CreateMode.PERSISTENT);
            }
            catch (KeeperException.NodeExistsException e)
            {
            }
        }
        //创建节点本身
        try {
            if (persistent) {
                zk.create(path, null, ZooDefs.Ids.OPEN_ACL_UNSAFE, CreateMode.PERSISTENT);
            } else {
                zk.create(path, null, ZooDefs.Ids.OPEN_ACL_UNSAFE, CreateMode.EPHEMERAL);
            }
        }
        catch (KeeperException.NodeExistsException e)
        {
        }


        //写数据
        zk.setData(path, data, -1);//如果不存在就抛出异常，他妈的搞的真不优雅
        return "success";





    }
    public byte[] read(String path) throws Exception
    {

        try {

            byte[] data = zk.getData(path, null, null);
            return data;
        }
        catch (KeeperException.NoNodeException e)
        {
            return null;
        }



    }



    public  void deleteRecursive( final String pathRoot)
            throws InterruptedException, KeeperException
    {
     //   PathUtils.validatePath(pathRoot);

        List<String> tree = listSubTreeBFS( pathRoot);

        for (int i = tree.size() - 1; i >= 0 ; --i) {
            //Delete the leaves first and eventually get rid of the root
            try {
                zk.delete(tree.get(i), -1); //Delete all versions of the node with -1.
            }
            catch (KeeperException.NoNodeException e)
            {
                //算成功哦
            }
            catch (Exception e)
            {
                throw  e;
            }

        }
    }





    private  List<String> listSubTreeBFS( final String pathRoot) throws
            KeeperException, InterruptedException {
        Deque<String> queue = new LinkedList<String>();
        List<String> tree = new ArrayList<String>();
        queue.add(pathRoot);
        tree.add(pathRoot);
        while (true) {
            String node = queue.pollFirst();
            if (node == null) {
                break;
            }
            try {
                List<String> children = zk.getChildren(node, false);
                for (final String child : children) {
                    final String childPath = node + "/" + child;
                    queue.add(childPath);
                    tree.add(childPath);
                }
            }
            catch (KeeperException.NoNodeException e) {
                //不存在算成功哦，可以删除不存在的node
            }
        }
        return tree;
    }
}
