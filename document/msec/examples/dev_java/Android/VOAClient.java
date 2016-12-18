import MainLogic.VOAJavaMainLogic;
import api.lb.msec.org.AccessLB;
import api.lb.msec.org.Route;
import org.msec.rpc.SrpcProxy;

import java.net.InetSocketAddress;
import java.net.Socket;

public class Main {

    public static void main(String[] args) {
        SrpcProxy proxy = new SrpcProxy();
        proxy.setCaller("android_1.0");//调用者身份，用于日志展现
        proxy.setMethod("MainLogic.MainLogicService.getTitles");//调用的RPC方法，基于协议里的package_name.service_name.rpc_name
        long seq = (long)(1000000 * Math.random());  //服务可自行替换Seq生成方法
        proxy.setSequence(seq);//请求的唯一标识，用于校验应答包，防止串包

        VOAJavaMainLogic.GetTitlesRequest.Builder requestBuilder = VOAJavaMainLogic.GetTitlesRequest.newBuilder();
        requestBuilder.setType("special");//请求包的参数初始化
        VOAJavaMainLogic.GetTitlesRequest request = requestBuilder.build();
        byte[] sendBytes = proxy.serialize(request);	//将请求包序列化为字节流，可通过网络发送。注意：序列化后的结果加上了SRPC的内部报文头
        VOAJavaMainLogic.GetTitlesResponse response;    //回包结构体






        //网络通信的基本工作
        Socket socket = new Socket();
        String ip = ""; // 服务器IP
        int port = 7963;

        try {
            /*
             * MSEC v2.0 released some api to access LB
             *
             * client such as servlet or cgi can call these api to get IP and port of destination service, and
             * if you do so, you must install LB agent on client machines, and start it in 'client' mode. for example:
             *                        ./nlbagent 10.104.95.110:8972 -m client -i eth0
             * You can get detail information from nlbagent program
             *
             * Actually, this is not fixed situation, servlet or cgi may have its own load balance method, MSEC can
             * not arrange their business.
             *
            AccessLB lb = new AccessLB();
            Route r = new Route();

            lb.getroutebyname("VOA_java.MainLogic", r);
            ip = r.getIp();
            port = r.getPort();

            System.out.println("get server from LB:"+ip+":"+port);
            */

            socket.setSoTimeout(20000);//20秒超时
            socket.connect(new InetSocketAddress(ip, port), 2000);  //连接服务

            //发送srpc请求
            socket.getOutputStream().write(sendBytes);

            //流式通信情况下，反复收应答数据，直到获得一个完整的应答报文
            byte[] buf = new byte[102400];
            int offset = 0;
            int result = 0;
            while(true)
            {
                int len = socket.getInputStream().read(buf, offset, buf.length-offset);
                if (len <= 0) {
                    throw new Exception("recv package failed");
                }
                offset += len;
                result = proxy.checkPackage(buf, offset);   //检查报文完整性
                if (result < 0) {//出错了                {
                    throw new Exception("check package failed");
                }
                else if (result > 0) //已经收到了一个完整的报文，长度为result
                    break;
            }
            response = (VOAJavaMainLogic.GetTitlesResponse)proxy.deserialize(buf, result, VOAJavaMainLogic.GetTitlesResponse.getDefaultInstance());	//解析报文
            if(response == null) {
                throw new Exception(String.format("Deserialize error: [%d]%s", proxy.getErrno(), proxy.getErrmsg()));
            }

            if( proxy.getSequence() != seq) {//sequence一致吗？
                throw new Exception("Sequence mismatch");
            }

            //业务层面的逻辑处理等
            for(int i = 0; i < response.getTitlesCount(); i++) {
                System.out.println("Resp title: "+ response.getTitles(i));
            }
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }
}
