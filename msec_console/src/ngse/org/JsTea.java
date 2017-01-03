
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

import javax.script.ScriptEngine;
import javax.script.ScriptEngineManager;
import javax.script.ScriptException;
import javax.servlet.ServletContext;
import javax.servlet.http.HttpServlet;
import java.io.*;

/**
 * Created by Administrator on 2016/2/11.
 * web前端用到了javascript写的tea加密算法，保存在/js/tea.js
 * 这个类是web后端java的方式来使用tea.js代码来实现tea密码算法
 */
public class JsTea {
    private ScriptEngine engine;

    //获取web项目里js文件目录所需
    //这个变量会在ServletConfig类初始化的时候被初始化一下
    static public ServletContext context = null;

    //构造函数
    //找到tea.js文件，并读入ScriptEngin引擎
    public JsTea(HttpServlet servlet)
    {
        ScriptEngineManager manager = new ScriptEngineManager();
        engine = manager.getEngineByName("javascript");
        InputStreamReader r = null;

        //各种情况下，尽量找到tea.js文件
        if (context != null)
        {
            InputStream in = context.getResourceAsStream("/js/tea.js");
            r = new InputStreamReader(in);
        }
        else if (servlet != null)
        {
            InputStream in = servlet.getServletContext().getResourceAsStream("/js/tea.js");
            r = new InputStreamReader(in);
        }
        else
        {
            File f = new File("D:\\ngse\\ngse_console\\web\\js\\tea.js");
            try {
                r = new InputStreamReader(new FileInputStream(f));
            }catch (Exception e) {}
        }



        //加载到引擎里
        try{
            engine.eval(r);
            r.close();


        }catch(ScriptException e){

            e.printStackTrace();
        }
        catch ( Exception e )
        {
            e.printStackTrace();
        }
    }
    //执行js代码s，并将结果保存在Object里
    public  Object eval(String s)
    {
        try {
            return engine.eval(s);
        }
        catch (ScriptException e)
        {
            e.printStackTrace();
            return e;
        }
    }
    //加密
    public String encrypt(String plainText, String key)
    {
        String s = String.format("encrypt('%s', '%s');", plainText, key);
        try {
            return engine.eval(s).toString();
        }
        catch (ScriptException e)
        {
            return "";
        }
    }
    //解密
    public String decrypt(String cipherText,String key)
    {
        String s = String.format("decrypt('%s', '%s');", cipherText, key);
        try {
            return engine.eval(s).toString();
        }
        catch (ScriptException e)
        {
            return "";
        }
    }
}
