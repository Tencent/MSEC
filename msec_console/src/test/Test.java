
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


package test;

import beans.dbaccess.IDL;
import beans.request.DelSecondLevelServiceIPInfoRequest;
import beans.request.IPPortPair;
import beans.service.DelSecondLevelServiceIPInfo;
import java.util.regex.*;
import ngse.org.AccessZooKeeper;

import junit.framework.TestCase;
import ngse.org.GzipUtil;
import ngse.org.RemoteShell;
import org.codehaus.jackson.map.ObjectMapper;

/**
 * Created by Administrator on 2016/1/25.
 */
public class Test extends TestCase {

    @org.junit.Test
    public void testFunc()
    {
        Pattern pattern = Pattern.compile("^frm\\..*$");
        Matcher matcher = pattern.matcher("frm.close tcp connection");

        if (matcher.find())
        {
            System.out.println("yes");
        }
        else
        {
            System.out.println("no");
        }

        matcher = pattern.matcher("frm close tcp connection");
        if (matcher.find())
        {
            System.out.println("yes");
        }
        else
        {
            System.out.println("no");
        }



    }


       /*
    @org.junit.Test
    public void testTEA()
    {

          String quote = "c5e172e2507e0a58fac8e343cdfd947d58e1aa1c";


        TEA tea = new TEA("bb05deaed2226fbe".getBytes());

        byte[] original = quote.getBytes();


        byte[] crypt = tea.encrypt(original);
        byte[] result = tea.decrypt(crypt);
        int i;
        for (i = 0; i < crypt.length; ++i)
        {
            System.out.print(String.format("%02x ", crypt[i]));
        }
        System.out.println();
        System.out.println("len:"+crypt.length);


        String test = new String(result);
        if (!test.equals(quote))
            throw new RuntimeException("Fail");
        return ;

    }

    @org.junit.Test
    public void testJS()
    {
        JsTea  tea = new JsTea(null);
        String key = "bb05deaed2226fbe77435292c1114088";
        String s = tea.encrypt("c5e172e2507e0a58fac8e343cdfd947d55c5967f", key);

        System.out.println(s);
        String ss = tea.decrypt(s, key);
        System.out.println("decrypt result:"+ss);


    }
     */



}
