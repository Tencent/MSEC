

<?php

/**
 * @brief 自动生成的业务代码逻辑实现
 * PHP版本实现部分
 */

require_once 'pb4php/message/pb_message.php';   // pb4php文件
require_once 'pb_proto_msec.php';   // 自动生成的pb文件
require_once 'call_service.php';                // callmethod文件

function callback($response)
{
    $len = strlen($response);
    if ($len < 10)
    {
        return 0;
    }
    $pkglen = intval(substr($response, 0, 10));// 10字节后面的内容的长度
    if ($pkglen+10 <= $len)
    {
        return $pkglen+10;
    }
    return 0;
}


class CrawlService{

    
    /**
     * @brief  自动生成的业务方法实现接口
     * @param  request  [入参]业务请求报文，可能是序列化后的protobuf或者json报文
     *         is_json  [入参]是否json报文
     * @return 业务回复报文，pb序列化后的报体
     */
    public function GetMP3List($request, $isJson)
    {
        if($isJson) {
        //not supported...
        $response = "{\"status\":-1000, \"msg\":\"Json not supported\"}";
        return $response;
        }
        /* pb部分，反序列化请求包体 */
        $req = new GetMP3ListRequest();
        $req->ParseFromString($request);
        $rsp = new GetMP3ListResponse();
        // TODO: 业务逻辑实现
        nglog_info("GetMP3List start...\n");
        attr_report('GetMP3List entry');
        if ( strlen($req->type()) < 1 ||
           strcmp($req->type(), "standard") != 0 && strcmp($req->type(), "special") != 0)
        {
            $rsp->set_msg("type field invalid");
            $rsp->set_status(100);
            goto label_end;
        }
        $jsonReq = sprintf("{\"handleClass\":\"com.bison.GetMP3List\",  \"requestBody\": {\"type\":\"%s\"} }",
            $req->type());
        $lenStr = sprintf("%-10d", strlen($jsonReq));
        $jsonReq = $lenStr.$jsonReq;
        nglog_info("begin to call jsoup:".$jsonReq."\n");
        $jsonResp = "";
        $errmsg = "";
        $ret = callmethod_odd("Jsoup.jsoup", $jsonReq, $jsonResp, $errmsg, "callback", 10000000);
        if ($ret != 0)
        {
            $rsp->set_msg($errmsg);
            $rsp->set_status(100);
            nglog_error("call jsoup failed:".$errmsg."\n");
            goto label_end;
        }
        nglog_info("call jsoup success\n");
        $jsonObj = json_decode(substr($jsonResp, 10));
        $status = $jsonObj->{'status'};
        if ($status != 0)
        {
            $rsp->set_msg("jsoup returns ".$status);
            $rsp->set_status(100);
            nglog_error("jsoup returns ".$status."\n");
            goto label_end;
        }
        nglog_info("jsoup returns ok\n");
        $mp3s = $jsonObj->{'mp3s'};
        $index = 0;
        nglog_info("begin to scan mp3 list...\n");
        foreach ($mp3s as $mp3)
        {
            if (strlen($mp3->{'title'}) < 5)
            {
                continue;
            }
            $oneMP3 = new OneMP3();
            $oneMP3->set_url($mp3->{'url'});
            $oneMP3->set_title($mp3->{'title'});
            nglog_info(sprintf("get one mp3:%s,%s\n",$mp3->{'url'}, $mp3->{'title'} ));
            $rsp->set_mp3s($index, $oneMP3);
            $index++;
        }
        $rsp->set_status(0);
        $rsp->set_msg("success");
        nglog_info("GetMP3List successfully\n");
        attr_report("GetMP3List successfully");
        $dbinfo =  getroutebyname("Database.mysql");
        $dbuser = "msec";
        $dbpass = "msec@anyhost";
        $dbname = "msec_test_java_db";
        $dbconn = mysqli_connect($dbinfo["ip"],$dbuser, $dbpass, $dbname, $dbinfo["port"]);
        if (!$dbconn)
        {
            nglog_error("db connect fails.\n");
            goto label_end;
        }
        $query = "INSERT INTO mp3_list(title, url) VALUES (?,?)";
        foreach ($mp3s as $mp3) {
            $stmt = $dbconn->prepare($query);
            $stmt ->bind_param("ss", $mp3->{'title'}, $mp3->{'url'});
            $stmt->execute();
            $stmt->close();
        }
        //close
        $dbconn->close();
        nglog_info("Update DB successfully\n");
label_end:
        /* 序列化回复包体 */
        $response = $rsp->serializeToString();
        return $response; 
    }
	
    
}

function callmethod_odd($servicename,
                        $request,
                        &$response,
                        &$errmsg,
                        $callback_func,
                        $timeout_usec)
{
    $route = getroutebyname($servicename);
    if ($route == null)
    {
        $errmsg = ("getroutebyname failed");
        return -1;
    }
    $comm_type = $route["type"];
    if ($comm_type == "tcp") {
        return callmethod_odd_tcp($route,
            $request ,$response, $errmsg, $callback_func, $timeout_usec);
    } else if ($comm_type == "udp") {
        return callmethod_odd_udp($route,
            $request ,$response, $errmsg, $timeout_usec);
    } else {
        $errmsg = ("unknown comm_type:" . $comm_type);
        return -1;
    }
}
function callmethod_odd_tcp(array  $route,
                        $request,
                        &$response,
                        &$errmsg,
                        $callback_func,
                        $to_usec)
{
    $ip = $route["ip"];
    $port = $route["port"];
    $fd = 0;
    $lefttime = $to_usec;
    $starttime = gettimeofday();
    $fd = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    socket_set_nonblock($fd);
    // connect to server
    $ret = socket_connect($fd, $ip, $port);
    if ($ret === FALSE) {
        $errno = socket_last_error();
        if ($errno != SOCKET_EALREADY && $errno != SOCKET_EINPROGRESS) {
            $errmsg = "connect failed:" . socket_strerror($errno);
            socket_close($fd);
            return -1;
        }
        $w = array($fd);
        $r = NULL;
        $e = NULL;
        $num = socket_select($r, $w, $e, $lefttime / 1000000, $lefttime % 1000000);
        if ($num === FALSE) {
            $errmsg = "select failed:" . socket_strerror(socket_last_error());
            socket_close($fd);
            return -1;
        }
        if ($num == 0) {
            $errmsg = "connect timeout";
            socket_close($fd);
            return -1;
        }
        $ret = socket_get_option($fd, SOL_SOCKET, SO_ERROR);
        if ($ret === FALSE || $ret != 0)
        {
            $errmsg = "connect failed";
            socket_close($fd);
            return -1;
        }
        // modify left time
        $now = gettimeofday();
        $lefttime = $to_usec;
        $lefttime -= ($now["sec"] - $starttime["sec"]) * 1000000 + $now["usec"] - $starttime["usec"];
        if ($lefttime < 10) {
            $errmsg = "time out";
            socket_close($fd);
            return -1;
        }
    }
    // connection is estabelished
    // write to server
    $ret = socket_write($fd, $request);
    if ($ret == NULL || $ret < strlen($request)) //strlen不会因为遇到字节0就停下来
    {
        $errmsg = "send failed:" . socket_strerror(socket_last_error());
        socket_close($fd);
        return -1;
    }
    // modify left time
    $now = gettimeofday();
    $lefttime = $to_usec;
    $lefttime -= ($now["sec"] - $starttime["sec"]) * 1000000 + $now["usec"] - $starttime["usec"];
    if ($lefttime < 10) {
        $errmsg = "time out";
        socket_close($fd);
        return -1;
    }
    $response = "";
    $resplen = 0;
    // recv with timeout
    while ($lefttime > 0) {
        $r = array($fd);
        $w = NULL;
        $e = NULL;
        $num = socket_select($r, $w, $e, $lefttime / 1000000, $lefttime % 1000000);
        if ($num === FALSE) {
            $errmsg = "select failed:" . socket_strerror(socket_last_error());
            socket_close($fd);
            return -1;
        }
        if ($num == 0) {
            $errmsg = "receive timeout";
            socket_close($fd);
            return -1;
        }
        $data = socket_read($fd, 65535);
        if ($data === FALSE) {
            $errmsg = "receive failed:" . socket_strerror(socket_last_error());
            socket_close($fd);
            return -1;
        }
        if ($data == "") // no more
        {
            $errmsg = "peer closed the connection";
            socket_close($fd);
            return -1;
        }
        $response = $response . $data;
        $ret = $callback_func($response);
        if ($ret == 0) // have NOT get a  complete package, continue
        {
        } else if ($ret > 0) //get a complete package, length is $ret
        {
            $resplen = $ret;
            if ($resplen > strlen($response)) {
                $errmsg = "callback function has bugs!";
                socket_close($fd);
                return -1;
            }
            break;
        } else  // package format invalid
        {
            $errmsg = "response package format error";
            socket_close($fd);
            return -1;
        }
        // modify left time
        $now = gettimeofday();
        $lefttime = $to_usec;
        $lefttime -= ($now["sec"] - $starttime["sec"]) * 1000000 + $now["usec"] - $starttime["usec"];
        if ($lefttime < 10) {
            $errmsg = "time out";
            socket_close($fd);
            return -1;
        }
    }
    // get a respons , its length is $resplen
    $response = substr($response, 0, $resplen);
    socket_close($fd);
    return 0;
}
function callmethod_odd_udp(array  $route,
                            $request,
                            &$response,
                            &$errmsg,
                            $to_usec)
{
    $ip = $route["ip"];
    $port = $route["port"];
    $fd = 0;
    $lefttime = $to_usec;
    $starttime = gettimeofday();
    $fd = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP);
    socket_set_nonblock($fd);
    // write to server
    $ret = socket_write($fd, $request);
    if ($ret == NULL || $ret < strlen($request)) //strlen不会因为遇到字节0就停下来
    {
        $errmsg = "send failed:" . socket_strerror(socket_last_error());
        socket_close($fd);
        return -1;
    }
    $response = "";
    $resplen = 0;
    // recv with timeout
    $r = array($fd);
    $w = NULL;
    $e = NULL;
    $num = socket_select($r, $w, $e, $lefttime / 1000000, $lefttime % 1000000);
    if ($num === FALSE) {
        $errmsg = "select failed:" . socket_strerror(socket_last_error());
        socket_close($fd);
        return -1;
    }
    if ($num == 0) {
        $errmsg = "receive timeout";
        socket_close($fd);
        return -1;
    }
    $response = socket_read($fd, 65535);
    if ($response === FALSE) {
        $errmsg = "receiv failed:" . socket_strerror(socket_last_error());
        socket_close($fd);
        return -1;
    }
    socket_close($fd);
    return 0;
}


?>

