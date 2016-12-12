package crawl;

import api.lb.msec.org.AccessLB;
import api.lb.msec.org.Route;
import api.log.msec.org.AccessLog;
import api.monitor.msec.org.AccessMonitor;
import com.google.protobuf.RpcController;
import com.google.protobuf.ServiceException;
import org.apache.log4j.Logger;
import org.json.JSONArray;
import org.json.JSONObject;
import org.msec.rpc.ServiceFactory;

import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.Charset;
import java.sql.SQLException;
import java.util.*;

public class ServiceImpl implements Msec.CrawlService.BlockingInterface {

    private static Logger log = Logger.getLogger(ServiceImpl.class.getName());
    public static void main(String[] args) throws Exception {
        ServiceFactory.initModule("VOA_java.Crawl");
    
        ServiceFactory.addService("crawl.CrawlService", Msec.CrawlService.BlockingInterface.class, new ServiceImpl());
    
        ServiceFactory.runService();
    }

    
    public Msec.GetMP3ListResponse getMP3List(RpcController controller, Msec.GetMP3ListRequest request) throws ServiceException {
	//Add your code here
        AccessMonitor.add("getMP3List_entry");
        String type = request.getType();
        AccessLog.doLog(AccessLog.LOG_LEVEL_DEBUG, "Req:" + request);
        Msec.GetMP3ListResponse.Builder builder = Msec.GetMP3ListResponse.newBuilder();
        if ( !(type.equals("special")||type.equals("standard")) )
        {
            builder.setStatus(100);
            builder.setMsg("invaid type field in request");
            return builder.build();
        }
        String jsonStr = String.format("{\"handleClass\":\"com.bison.GetMP3List\",  \"requestBody\": {\"type\":\"%s\"} }",
                type);
        String lenStr = String.format("%-10d", jsonStr.getBytes().length);

        AccessLB lb = new AccessLB();
        Route r = new Route();
        Socket socket = new Socket();

        DBUtil util = null;
        try {
            lb.getroutebyname("Database.mysql", r);
            util = new DBUtil(r.getIp()+":"+r.getPort());
            if (util.getConnection() == null)
            {
                builder.setStatus(100);
                builder.setMsg("db connect failed!");
                AccessMonitor.add("connect_mysql_fail");
                return builder.build();
            }
            AccessMonitor.add("connect_mysql_succ");

            //get route information
            lb.getroutebyname("Jsoup.jsoup", r);
            if (r.getComm_type() != Route.COMM_TYPE.COMM_TYPE_TCP &&
                    r.getComm_type() != Route.COMM_TYPE.COMM_TYPE_ALL)
            {
                builder.setStatus(100);
                builder.setMsg("tcp is not supported by jsoup");
                return builder.build();
            }

            // connect server
            socket.setSoTimeout(20000);//20 seconds
            socket.connect(new InetSocketAddress(r.getIp(), r.getPort()), 2000);

            // send request bytes
            socket.getOutputStream().write(lenStr.getBytes(Charset.forName("utf8")));
            socket.getOutputStream().write(jsonStr.getBytes(Charset.forName("utf8")));

            // recv response bytes
            byte[] buf = new byte[102400];
            int max = 10;
            int total = 0;
            while (total < max)
            {
                int len = socket.getInputStream().read(buf, total, max-total);
                if (len <= 0)
                {
                    socket.close();
                    throw new Exception("recv json length failed");
                }
                total += len;
            }
            max = new Integer(new String(buf, 0, 10, Charset.forName("utf8")).trim()).intValue();
            total = 0;
            while (total < max)
            {
                int len = socket.getInputStream().read(buf, total, max-total);
                if (len <= 0)
                {
                    socket.close();
                    throw new Exception("recv json bytes failed");
                }
                total += len;
            }

            // parse the response json
            JSONObject jsonObject = new JSONObject(new  String(buf, 0, total, Charset.forName("utf8")));
            int status = jsonObject.getInt("status");
            if (status != 0)
            {
                throw  new Exception("json string status:"+status);
            }
            JSONArray mp3s = jsonObject.getJSONArray("mp3s");
            for (int i = 0; i < mp3s.length() ; i++) {
                JSONObject mp3 = mp3s.getJSONObject(i);
                String title = mp3.getString("title");
                String url = mp3.getString("url");

                Msec.OneMP3.Builder bb = Msec.OneMP3.newBuilder();
                bb.setTitle(title);
                bb.setUrl(url);
                builder.addMp3S(bb.build());

                String sql = "insert into mp3_list(title, url) values(?,?)";
                List<Object> params = new ArrayList<Object>();
                params.add(title);
                params.add(url);
                try {
                    int addNum = util.updateByPreparedStatement(sql, params);
                    if (addNum < 0) {
                        builder.setMsg("db add record failed.");
                        builder.setStatus(100);
                        return builder.build();
                    }
                    AccessMonitor.add("access_mysql_succ");
                } catch (SQLException e) {
                    builder.setMsg("db add record failed:" + e.toString());
                    builder.setStatus(100);
                    e.printStackTrace();
                    return builder.build();
                }
            }
            AccessMonitor.add("access_jsoup_succ");
            AccessLog.doLog(AccessLog.LOG_LEVEL_INFO, "Resp OK:"+ mp3s.length());
            AccessLog.doLog(AccessLog.LOG_LEVEL_ERROR, "Resp OK");
        }
        catch (Exception e) {
            e.printStackTrace();
            builder.setStatus(100);
            builder.setMsg(e.getMessage());
            AccessMonitor.add("getMP3List_fail");
            return builder.build();
        }
        finally {
            try {
                socket.close();
            } catch (Exception e){}
            if(util != null && util.getConnection() != null) {
                util.releaseConn();
            }
        }

        builder.setStatus(0);
        builder.setMsg("success");
        return builder.build();
    }
}
