package org.msec.sink.es;

import org.elasticsearch.action.bulk.BulkRequestBuilder;
import org.elasticsearch.action.bulk.BulkResponse;
import org.elasticsearch.client.transport.TransportClient;
import org.elasticsearch.common.settings.Settings;
import org.elasticsearch.common.transport.InetSocketTransportAddress;
import org.elasticsearch.transport.client.PreBuiltTransportClient;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.BlockingQueue;


public class ESClientThread implements Runnable {
    private Logger LOG = LoggerFactory.getLogger(ESClientThread.class);
    private BlockingQueue<ESClientThread.ESThreadRequest> queue = null;

    private String clusterName;
    private InetSocketTransportAddress serverAddresses;
    private TransportClient client = null;

    public ESClientThread(BlockingQueue<ESClientThread.ESThreadRequest> queue, String clusterName,
                          InetSocketTransportAddress serverAddresses)
    {
        this.queue = queue;

        this.clusterName = clusterName;
        this.serverAddresses = serverAddresses;

        Settings settings = Settings.builder().put("cluster.name", clusterName).
                put("client.transport.sniff", true).build();
        client = new PreBuiltTransportClient(settings);
        client.addTransportAddress(this.serverAddresses);
    }

    @Override
    public void run() {
        // TODO Auto-generated method stub
        try
        {
            while (true)
            {
                ESClientThread.ESThreadRequest  request = queue.take();
                BulkRequestBuilder bulkRequest = client.prepareBulk();
                for (int i=0; i<request.sourceList.size(); i++) {
                    //System.out.println("take from queue: " + source);
                    bulkRequest.add(client.prepareIndex(request.indexNameList.get(i), request.indexTypeList.get(i))
                            .setSource(request.sourceList.get(i)));
                    LOG.info("taken source: " + request.sourceList.get(i));
                }
                BulkResponse bulkResponse = bulkRequest.execute().actionGet();
                if (bulkResponse.hasFailures()) {
                    System.out.println("bulk response errorss!" + bulkResponse.buildFailureMessage());
                    bulkResponse = bulkRequest.execute().actionGet();
                }
                //Thread.sleep(10);
            }
        }
        catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static final class ESThreadRequest {
        public List<String>  indexNameList = new ArrayList<String>();
        public List<String>  indexTypeList = new ArrayList<String>();
        public List<String>  sourceList = new ArrayList<String>();

        @Override
        public String toString() {
            return "ESThreadRequest{" +
                    "indexNameList=" + indexNameList +
                    ", indexTypeList=" + indexTypeList +
                    ", sourceList=" + org.apache.commons.lang.StringUtils.join(sourceList.toArray(), ',') +
                    '}';
        }

        public void clear() {
            indexNameList.clear();
            indexTypeList.clear();
            sourceList.clear();
        }
    }
}
