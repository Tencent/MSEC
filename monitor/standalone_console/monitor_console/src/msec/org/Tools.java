
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


package msec.org;

import beans.response.OneAttrDaysChart;
import org.jfree.chart.*;

import org.jfree.chart.axis.*;
import org.jfree.chart.plot.XYPlot;
import org.jfree.chart.title.TextTitle;
import org.jfree.data.time.*;
import org.jfree.data.xy.XYDataset;
import beans.response.OneDayValue;
import org.jfree.ui.RectangleInsets;

import javax.imageio.ImageIO;
import java.awt.*;
import java.awt.image.BufferedImage;
import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.security.MessageDigest;
import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.*;

/**
 * Created by Administrator on 2016/2/10.
 */
public class Tools {
    private final static GregorianCalendar gc = new GregorianCalendar(2016,0,1);
    private static XYDataset createDataset( OneDayValue[] oneDayValues) {
        TimeSeriesCollection timeseriescollection = new TimeSeriesCollection();

        for (int j = 0; j < oneDayValues.length; j++) {

            TimeSeries timeseries = new TimeSeries(oneDayValues[j].getDate());

            Minute current = new Minute(gc.getTime());
            int[] data = oneDayValues[j].getValues();

            int len = data.length;
            //if it is today... check actual length for data
            if(oneDayValues[j].getDate().equals(Tools.nowString("yyyyMMdd")) ) {
                Calendar cal = Calendar.getInstance();
                len = cal.get(Calendar.HOUR_OF_DAY)*60+cal.get(Calendar.MINUTE);
            }

            for (int i = 0; i < len; ++i) {
                timeseries.add(current, (double) (data[i]));
                current = (Minute) current.next();
                if(data[i] > oneDayValues[0].getMax())
                    oneDayValues[0].setMax(data[i]);
            }
            timeseriescollection.addSeries(timeseries);
        }

        return timeseriescollection;
    }
    private static XYDataset createDaysDataset( ArrayList<OneDayValue> oneDayValues, GregorianCalendar startgc, OneAttrDaysChart chart) {
        TimeSeriesCollection timeseriescollection = new TimeSeriesCollection();
        TimeSeries timeseries = new TimeSeries(oneDayValues.get(0).getDate()+"-"+oneDayValues.get(oneDayValues.size()-1).getDate());
        int sum = 0;
        int max = 0;
        int min = 0;
        Minute current = new Minute(startgc.getTime());
        for (int j = 0; j < oneDayValues.size(); j++) {
            int[] data = oneDayValues.get(j).getValues();
            //check actual length for data
            int len = data.length;
            if(j == oneDayValues.size()-1) {
                //if last day is today...
                if(oneDayValues.get(j).getDate().equals(Tools.nowString("yyyyMMdd"))) {
                    Calendar cal = Calendar.getInstance();
                    len = cal.get(Calendar.HOUR_OF_DAY)*60+cal.get(Calendar.MINUTE);
                }
            }
            for (int i = 0; i < len; ++i) {
                timeseries.add(current, (double) (data[i]));
                sum += data[i];
                max = Math.max(max, data[i]);
                if(min == 0)
                    min = data[i];
                else if(data[i] != 0)
                    min = Math.min(min, data[i]);
                current = (Minute) current.next();
            }
        }
        chart.setMax(max);
        chart.setMin(min);
        chart.setSum(sum);
        timeseriescollection.addSeries(timeseries);
        return timeseriescollection;
    }

    public static String generateFullDayChart(String filename, OneDayValue[] data, String title)
    {
        if (data[0].getValues().length != 1440 )
        {
            return "data size invalid";
        }
        if (data.length > 1)
        {
            if (data[1].getValues() == null || data[1].getValues().length != 1440 )
            {
                return "data 1 invalid";
            }
        }

        XYDataset xydataset = createDataset(data);
        JFreeChart jfreechart = ChartFactory.createTimeSeriesChart(title,
                "time", "", xydataset, true, true, true);
        //jfreechart.getRenderingHints().put(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_OFF);

        try {
            XYPlot xyplot = (XYPlot) jfreechart.getPlot();

            //线条
            xyplot.setRangeGridlinePaint(ChartColor.GRAY);
            xyplot.setBackgroundPaint(ChartColor.WHITE);
            xyplot.setAxisOffset(new RectangleInsets(0, 0, 0, 0));
            xyplot.setOutlinePaint(null);//去掉边框

            //横轴
            DateAxis dateaxis = (DateAxis) xyplot.getDomainAxis();
            dateaxis.setDateFormatOverride(new SimpleDateFormat("H"));
            dateaxis.setLabelFont(new Font("微软雅黑", Font.PLAIN, 14)); //水平底部标题
            dateaxis.setLabelPaint(ChartColor.black);
            dateaxis.setTickLabelFont(new Font("微软雅黑", Font.PLAIN, 14));
            dateaxis.setTickLabelPaint(ChartColor.black);

            GregorianCalendar endgc = (GregorianCalendar)gc.clone();
            endgc.add(GregorianCalendar.DATE, 1);
            dateaxis.setMaximumDate(endgc.getTime());
            dateaxis.setTickMarksVisible(true);
            dateaxis.setTickMarkInsideLength(5);
            dateaxis.setTickUnit(new DateTickUnit(DateTickUnitType.HOUR, 2));
            //dateaxis.setVerticalTickLabels(true);
            dateaxis.setLabel("");

            //纵轴
            ValueAxis rangeAxis = xyplot.getRangeAxis();//获取柱状
            rangeAxis.setLabelFont(new Font("微软雅黑", Font.PLAIN, 14));
            rangeAxis.setLabelPaint(ChartColor.black);
            rangeAxis.setTickLabelFont(new Font("微软雅黑", Font.PLAIN, 14));
            rangeAxis.setTickLabelPaint(ChartColor.black);
            rangeAxis.setLowerBound(0);
            rangeAxis.setUpperBound(Tools.upperBound(data[0].getMax()));

            NumberAxis numAxis = (NumberAxis) rangeAxis;
            numAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());

            //图例
            jfreechart.getLegend().setItemFont(new Font("微软雅黑", Font.PLAIN, 12));
            jfreechart.getLegend().setItemPaint(ChartColor.black);
            jfreechart.getLegend().setBorder(0, 0, 0, 0);//去掉边框

            //标题
            jfreechart.getTitle().setFont(new Font("微软雅黑", Font.BOLD, 16));//设置标题字体
            jfreechart.getTitle().setPaint(ChartColor.black);

            int w = 500;
            int h = 300;

            ChartUtilities.saveChartAsPNG(new File(filename), jfreechart, w, h);
            //ChartUtilities.saveChartAsJPEG(new File(filename),0.8f, jfreechart, w, h);

            return "success";

        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
    }

    public static String generateDaysChart(String filename, ArrayList<OneDayValue> data, OneAttrDaysChart chart, String title, int duration)
    {
        if (data.size() == 0)
        {
            return "data size invalid";
        }

        int date = Integer.parseInt(data.get(0).getDate());
        GregorianCalendar startgc = new GregorianCalendar(date/10000, date%10000/100-1, date%100);

        XYDataset xydataset = createDaysDataset(data, startgc, chart);
        JFreeChart jfreechart = ChartFactory.createTimeSeriesChart(title,
                "time", "", xydataset, true, true, true);
        //jfreechart.getRenderingHints().put(RenderingHints.KEY_TEXT_ANTIALIASING,RenderingHints.VALUE_TEXT_ANTIALIAS_OFF);

        try {
            XYPlot xyplot = (XYPlot) jfreechart.getPlot();
            //线条
            xyplot.setRangeGridlinePaint(ChartColor.GRAY);
            xyplot.setBackgroundPaint(ChartColor.WHITE);
            xyplot.setAxisOffset(new RectangleInsets(0, 0, 0, 0));
            xyplot.setOutlinePaint(null);//去掉边框
            //横轴
            DateAxis dateaxis = (DateAxis) xyplot.getDomainAxis();
            dateaxis.setDateFormatOverride(new SimpleDateFormat("MM/dd"));

            dateaxis.setLabelFont(new Font("微软雅黑", Font.PLAIN, 14)); //水平底部标题
            dateaxis.setLabelPaint(ChartColor.black);
            dateaxis.setTickLabelFont(new Font("微软雅黑", Font.PLAIN, 14));
            dateaxis.setTickLabelPaint(ChartColor.black);

            dateaxis.setMinimumDate(startgc.getTime());
            GregorianCalendar endgc = (GregorianCalendar)startgc.clone();
            endgc.add(GregorianCalendar.DATE, duration);
            dateaxis.setMaximumDate(endgc.getTime());

            dateaxis.setTickMarksVisible(true);
            dateaxis.setTickMarkInsideLength(5);
            dateaxis.setTickUnit(new DateTickUnit(DateTickUnitType.DAY, 1));
            //dateaxis.setVerticalTickLabels(true);
            dateaxis.setLabel("");

            //纵轴
            ValueAxis rangeAxis = xyplot.getRangeAxis();//获取柱状
            rangeAxis.setLabelFont(new Font("微软雅黑", Font.PLAIN, 14));
            rangeAxis.setLabelPaint(ChartColor.black);
            rangeAxis.setTickLabelFont(new Font("微软雅黑", Font.PLAIN, 14));
            rangeAxis.setTickLabelPaint(ChartColor.black);
            rangeAxis.setLowerBound(0);
            rangeAxis.setUpperBound(Tools.upperBound(chart.getMax()));
            NumberAxis numAxis = (NumberAxis) rangeAxis;
            numAxis.setStandardTickUnits(NumberAxis.createIntegerTickUnits());

            //图例
            jfreechart.getLegend().setItemFont(new Font("微软雅黑", Font.PLAIN, 12));
            jfreechart.getLegend().setItemPaint(ChartColor.black);
            jfreechart.getLegend().setBorder(0,0,0,0);//去掉边框

            //标题
            jfreechart.getTitle().setFont(new Font("微软雅黑", Font.BOLD, 16));//设置标题字体
            jfreechart.getTitle().setPaint(ChartColor.black);


            int w = 500;
            int h = 300;
            ChartUtilities.saveChartAsPNG(new File(filename), jfreechart, w, h);
            //ChartUtilities.saveChartAsJPEG(new File(filename),0.8f, jfreechart, w, h);

            return "success";

        }
        catch (Exception e)
        {
            e.printStackTrace();
            return e.getMessage();
        }
    }

    static public int upperBound(int i)
    {
        if(i < 0)
            return i;
        int p = (int)Math.pow(10, Integer.toString(i).length()-1);
        return (i/p+1)*p;
    }

    static public  String randInt()
    {
        return String.format("%d", (int)(Math.random()*Integer.MAX_VALUE));
    }

    //长度整数转化为大端四字节整数
    static public byte[] int2Bytes(int i)
    {
        byte[] b = new byte[4];
        int v = 256 * 256 * 256;
        for (int j = 0; j < 3; j++) {
            b[j] = (byte)(i / v);
            i = i % v;
            v = v / 256;
        }
        b[3] = (byte)i;

        return b;
    }
    static public int bytes2int(byte[] buf)
    {
        int v = 0;
        int b0 = buf[0]; if (b0 < 0) { b0 += 256;}
        int b1 = buf[1]; if (b1 < 0) { b1 += 256;}
        int b2 = buf[2]; if (b2 < 0) { b2 += 256;}
        int b3 = buf[3]; if (b3 < 0) { b3 += 256;}
        v = b0 * (256*256*256) + b1 * (256*256) + b2*256 + b3;
        return v;
    }


    //分号分割的字符串分割为字符串列表
    public static ArrayList<String> splitBySemicolon(String s)
    {
        ArrayList<String> ret = new ArrayList<String>();
        int fromIndex = 0;
        while (true)
        {
            int index = s.indexOf(";", fromIndex);
            if (index >= 0)
            {
                String sub = s.substring(fromIndex, index);
                if (sub.length() > 0)
                {
                    ret.add(sub);
                }
                fromIndex = index + 1;
            }
            else
            {
                if (fromIndex < s.length())
                {
                    String sub = s.substring(fromIndex);
                    if (sub.length() > 0)
                    {
                        ret.add(sub);
                    }
                }
                break;
            }
        }
        return ret;
    }


    static public int[] zeroIntArray(int size)
    {
        int [] ret = new int[size];
        for (int i = 0; i < ret.length; i++) {
            ret[i] = 0;
        }
        return  ret;
    }

    static public String getPreviousDate(String date, int beforeday)
    {
        if(beforeday == 0)
            return date;
        int d = Integer.parseInt(date);
        Calendar cal = Calendar.getInstance();
        cal.set(d/10000, d%10000/100-1,d%100);
        cal.add(Calendar.DAY_OF_YEAR, beforeday*-1);
        SimpleDateFormat format = new SimpleDateFormat("yyyyMMdd");
        return format.format(cal.getTime());
    }

    //fork进程执行命令
    // cmd: 命令文件
    // sb： 标准输出和标准错误输出的内容保存，可以为null
    // waitFlag：是否等子进程结束再函数返回
    static public int runCommand(String[] cmd, StringBuffer sb, boolean waitflag )
    {

        Process pid = null;
        ProcessBuilder build = new ProcessBuilder(cmd);
        build.redirectErrorStream(true);
        try {
            pid = build.start();
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return -1;
        }
        if (sb != null) {
            //BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(pid.getInputStream()), 1024);
            InputStream in = pid.getInputStream();
            byte[] buf = new byte[10240];
            try {
                while (true)
                {
                    int len = in.read(buf);
                    if (len <= 0)
                    {
                        break;
                    }
                    sb.append(new String(buf, 0, len));
                }
            }
            catch (Exception e)   { }

        }
        if (waitflag) {
            try {
                pid.waitFor();
                int v = pid.exitValue();
                pid.destroy();
                return v;
            }catch (Exception e ){}
        }
        return 0;
    }
    //删除目录，目录可以为非空，递归的方式删除子项
    static public boolean deleteDirectory(File path)
    {

        if( path.exists() ) {
            File[] files = path.listFiles();
            for(int i=0; i<files.length; i++) {
                if(files[i].isDirectory()) {
                    deleteDirectory(files[i]);
                }
                else {
                    files[i].delete();
                }
            }
        }
        return( path.delete() );

    }

    static public String md5(String s)
    {
        try {
            MessageDigest md = MessageDigest.getInstance("MD5");

            md.update(s.getBytes());
            byte[] result = md.digest();
            if (result.length != 16)
            {
                return "";
            }

            return Tools.toHexString(result);// 32 bytes



        }
        catch (Exception e )
        {
            return "";
        }
    }
    static public String toHexString(byte[] b)
    {
        int i;
        StringBuffer sb = new StringBuffer();
        char[] chars = {'0', '1','2','3', '4','5','6','7','8','9','a','b','c','d','e','f'};
        for (i = 0; i < b.length; ++i)
        {
            int bb = b[i];
            if (bb < 0) { bb += 256;}
            int index;
            index = bb>>4;
            sb.append(chars[index]);
            index = bb & 0x0f;
            sb.append(chars[index]);
        }
        return sb.toString();
    }
    static private  int hexChr2Int(char c)
    {
        char[] chars = {'0', '1','2','3', '4','5','6','7','8','9','a','b','c','d','e','f'};
        int i;
        for (i = 0; i < chars.length; ++i)
        {
            if (chars[i] == c)
            {
                return i;
            }
        }
        return 16;
    }
    static public byte[] fromHexString(String s)
    {
        int i;
        if ((s.length() % 2) != 0)
        {
            return new byte[0];
        }
        int len = s.length() / 2;
        byte[] b = new byte[len];


        for (i = 0; i < b.length; ++i)
        {
            int v1 = hexChr2Int(s.charAt(2*i));
            int v2 = hexChr2Int(s.charAt(2*i+1));
            if (v1 > 15 || v2 > 15) { return new byte[0];}
            b[i] = (byte)(v1*16+v2);
        }
        return b;
    }
    static public String nowString(String fmt)
    {
        // "yyyy-MM-dd HH:mm:ss"
        if (fmt == null || fmt.length() < 1)
        {
            fmt = "yyyy-MM-dd HH:mm:ss";
        }
        SimpleDateFormat df = new SimpleDateFormat(fmt); //设置日期格式
        return df.format(new Date());

    }
    static public String TimeStamp2DateStr(Long epochSecond, String formats){
        String date = new java.text.SimpleDateFormat(formats).format(new java.util.Date(epochSecond * 1000));
        return date;
    }
    static public String TimeStamp2DateStr(Long epochSecond)
    {
        return TimeStamp2DateStr(epochSecond,  "yyyy-MM-dd HH:mm:ss");
    }
}
