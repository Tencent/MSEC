
package MainLogic;

import api.log.msec.org.AccessLog;
import api.monitor.msec.org.AccessMonitor;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.RpcController;
import com.google.protobuf.ServiceException;
import org.msec.rpc.ServiceFactory;
import crawl.*;

public class ServiceImpl implements Msec.MainLogicService.BlockingInterface {

    public static void main(String[] args) throws Exception {
        ServiceFactory.initModule("VOA_java.MainLogic");
    
        ServiceFactory.addService("MainLogic.MainLogicService", Msec.MainLogicService.BlockingInterface.class, new ServiceImpl());
    
        ServiceFactory.runService();
    }

    
    public Msec.GetTitlesResponse getTitles(RpcController controller, Msec.GetTitlesRequest request) throws ServiceException {
        VOAJavaCrawl.GetMP3ListRequest.Builder requestBuilder = VOAJavaCrawl.GetMP3ListRequest.newBuilder();

        AccessMonitor.add("getTitiles_entry");

        //Add your code here: build your request
        requestBuilder.setType("special");
        Msec.GetTitlesResponse.Builder responseBuilder = Msec.GetTitlesResponse.newBuilder();


        VOAJavaCrawl.GetMP3ListRequest r = requestBuilder.build();


        AccessLog.doLog(AccessLog.LOG_LEVEL_DEBUG, "Req:" + request);

        try {
            AccessMonitor.add("callmethod(CrawlService.getMP3List");
            VOAJavaCrawl.GetMP3ListResponse response = (VOAJavaCrawl.GetMP3ListResponse) ServiceFactory.callMethod(
					"VOA_java.Crawl", 
					"crawl.CrawlService.getMP3List",
                    r, 
					VOAJavaCrawl.GetMP3ListResponse.getDefaultInstance(), 
					30000);
                
            if (response.getStatus()!= 0)
            {
                throw new Exception("VOA_java.Crawl status:"+response.getStatus());
            }

            for (int i = 0; i < response.getMp3SCount() ; i++) {
                VOAJavaCrawl.OneMP3 oneMP3 = response.getMp3S(i);
                String title = oneMP3.getTitle();
                responseBuilder.addTitles(title);
            }

            AccessLog.doLog(AccessLog.LOG_LEVEL_INFO, "Resp OK:"+ response.getMp3SCount());
            AccessLog.doLog(AccessLog.LOG_LEVEL_ERROR, "Resp OK");

        } catch (Exception ex) {
            ex.printStackTrace();
            responseBuilder.setMsg(ex.getMessage());
            responseBuilder.setStatus(100);
            AccessMonitor.add("getTitiles_fail");
            return responseBuilder.build();

        }
        AccessMonitor.add("getTitiles_succ");
        responseBuilder.setMsg("success");
        responseBuilder.setStatus(0);
        return responseBuilder.build();
    }
    
    public Msec.GetUrlByTitleResponse getUrlByTitle(RpcController controller, Msec.GetUrlByTitleRequest request) throws ServiceException {
        VOAJavaCrawl.GetMP3ListRequest.Builder requestBuilder = VOAJavaCrawl.GetMP3ListRequest.newBuilder();
        AccessMonitor.add("getUrlByTitle_entry");
        requestBuilder.setType("special");
        Msec.GetUrlByTitleResponse.Builder responseBuilder = Msec.GetUrlByTitleResponse.newBuilder();
        VOAJavaCrawl.GetMP3ListRequest r = requestBuilder.build();
        try {
            AccessMonitor.add("callmethod(CrawlService.getMP3List");
            VOAJavaCrawl.GetMP3ListResponse response = (VOAJavaCrawl.GetMP3ListResponse) ServiceFactory.callMethod("VOA_java.Crawl", "crawl.CrawlService.getMP3List",
                    r, VOAJavaCrawl.GetMP3ListResponse.getDefaultInstance(), 30000);

            // System.out.println("Request:\n" + request + "Response:\n" + response);
            if (response.getStatus()!= 0)
            {
                throw new Exception("VOA_java.Crawl status:"+response.getStatus());
            }
            for (int i = 0; i < response.getMp3SCount() ; i++) {
                VOAJavaCrawl.OneMP3 oneMP3 = response.getMp3S(i);
                String title = oneMP3.getTitle();
                if (title.equals(request.getTitle())) {
                    responseBuilder.setUrl(oneMP3.getUrl());
                    break;
                }
            }
            if (!responseBuilder.hasUrl())
            {
                throw new Exception("failed to find url for "+request.getTitle());
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            responseBuilder.setMsg(ex.getMessage());
            responseBuilder.setStatus(100);
            AccessMonitor.add("getUrlByTitle_fail");
            return responseBuilder.build();

        }
        AccessMonitor.add("getUrlByTitle_succ");
        responseBuilder.setMsg("success");
        responseBuilder.setStatus(0);
        return responseBuilder.build();
    }
    public Msec.DownloadMP3Response downloadMP3(RpcController controller, Msec.DownloadMP3Request request) throws ServiceException {
    //Add your code here
        return null;
    }
}
