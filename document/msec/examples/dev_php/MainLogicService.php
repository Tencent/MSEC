

<?php

/**
 * @brief 自动生成的业务代码逻辑实现
 * PHP版本实现部分
 */

require_once 'pb4php/message/pb_message.php';   // pb4php文件
require_once 'pb_proto_msec.php';   // 自动生成的pb文件
require_once 'call_service.php';                // callmethod文件
require_once 'pb_proto_VOA_php_Crawl.php';

class MainLogicService{

    

    public function GetTitles($request, $is_json)
    {
        nglog_info("GetTitles start....\n");
        attr_report("GetTitles Entry");

        $req = new GetTitlesRequest();
        if($is_json)
        {
            $req_array =json_decode($request);
            $req->set_type( $req_array->{"type"});
        }
        else {
            /* 反序列化请求包体 */
            $req->ParseFromString($request);
        }
        $rsp = new GetTitlesResponse();
        $rsp_array = array();
        $getMP3ListRequest = new GetMP3ListRequest();
        $getMP3ListRequest->set_type("special");
        $getMP3ListResp = new GetMP3ListResponse();
        $ret = callmethod("VOA_php.Crawl", "crawl.CrawlService.GetMP3List",
                $getMP3ListRequest->serializeToString(), 15000);
        if ($ret['ret'] != 0 )
        {
            $rsp->set_msg('callmethod failed:'.$ret['errmsg']);
            $rsp->set_status(100);
            $rsp_array["msg"]='callmethod failed:'.$ret['errmsg'];
            $rsp_array["status"]=100;
            nglog_error('callmethod failed:'.$ret['errmsg']."\n");
            goto  label_end;
        }
        nglog_info("call GetMP3List success\n");
        $getMP3ListResp->ParseFromString($ret['rsp']);
        if ($getMP3ListResp->status() != 0)
        {
            $rsp->set_msg('rpc return failed:'.$getMP3ListResp->msg());
            $rsp->set_status(100);
            $rsp_array["msg"]='rpc return failed:'.$getMP3ListResp->msg();
            $rsp_array["status"]=100;
            nglog_error('rpc return failed:'.$getMP3ListResp->msg()."\n");
            goto  label_end;
        }
        nglog_info("scan mp3 list...\n");
        $rsp_array["title"]=array();
        for ($i = 0; $i < $getMP3ListResp->mp3s_size(); $i++)
        {
            $oneMP3 =  $getMP3ListResp->mp3s($i);
            $title = $oneMP3->title();
            $rsp->append_titles($title);
            $rsp_array["title"][$i]=$title;
            nglog_info("append title:".$title."\n");
        }
        nglog_info("GetTitles end successfully.\n");
        attr_report("GetTitles Success");
        $rsp->set_msg("success");
        $rsp->set_status(0);
        $rsp_array["msg"]='success';
        $rsp_array["status"]=0;
label_end:
        /* 序列化回复包体 */
        if($is_json) {
            $response = json_encode($rsp_array);
            return $response;
        }
        else {
            $response = $rsp->serializeToString();
            return $response;
        }
    }


	
    

    public function GetUrlByTitle($request, $is_json)
    {
        $req = new GetUrlByTitleRequest();
        if($is_json)
        {
            $req_array=json_decode($request);

            $req->set_type($req_array->{"type"});
            $req->set_title($req_array->{"title"});
        }
        else {
            /* 反序列化请求包体 */
            $req->ParseFromString($request);
        }
        $rsp = new GetUrlByTitleResponse(); 
        $rsp_array = array();
        // TODO: 业务逻辑实现
        nglog_info("GetUrlByTitle start....\n");
        attr_report("GetUrlByTitle Entry");
        $getMP3ListRequest = new GetMP3ListRequest();
        $getMP3ListRequest->set_type("special");
        $getMP3ListResp = new GetMP3ListResponse();
        $ret = callmethod("VOA_php.Crawl", "crawl.CrawlService.GetMP3List",
                $getMP3ListRequest->serializeToString(), 15000);
        if ($ret['ret'] != 0)
        {
            $rsp->set_msg('callmethod failed:'.$ret['errmsg']);
            $rsp->set_status(100);
            $rsp_array["msg"] = 'callmethod failed:'.$ret['errmsg'];
            $rsp_array["status"] = 100;
            nglog_error('callmethod failed:'.$ret['errmsg']."\n");
            goto  label_end;
        }
        nglog_info("callmethod successfully\n");
        $getMP3ListResp->ParseFromString($ret['rsp']);
        if ($getMP3ListResp->status() != 0)
        {
            $rsp->set_msg('rpc failed:'.$getMP3ListResp->msg());
            $rsp->set_status(100);
            $rsp_array["msg"] = 'rpc failed:'.$getMP3ListResp->msg();
            $rsp_array["status"] = 100;
            nglog_error('rpc failed:'.$getMP3ListResp->msg()."\n");
            goto  label_end;
        }
        nglog_info("rpc return status=0\n");
        $rsp->set_msg("failed to find the url");
        $rsp->set_status(100);
        $rsp_array["msg"] = "failed to find the url";
        $rsp_array["status"] = 100;
        nglog_info("begin scan mp3 list...");
        for ($i = 0; $i < $getMP3ListResp->mp3s_size(); $i++)
        {
            $oneMP3 =  $getMP3ListResp->mp3s($i);
            //settype($oneMP3, "object");
            $title = $oneMP3->title();
            $url = $oneMP3->url();
            if ($req->title() === $title)
            {
                $rsp->set_url($url);
                $rsp->set_msg("success");
                $rsp->set_status(0);

                $rsp_array["msg"] = 'success';
                $rsp_array["status"] = 0;
                $rsp_array["url"] = $url;

                nglog_info("GetUrlByTitle end successfully.\n");
                attr_report("GetUrlByTitle Success");
                break;
            }
        }
 label_end:
        /* 序列化回复包体 */
        if($is_json) {
            $response = json_encode($rsp_array);
            return $response;
        }
        else {
            $response = $rsp->serializeToString();
            return $response;
        }
    }
	
    
}

?>

