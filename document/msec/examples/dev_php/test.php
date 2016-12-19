<?php
require_once 'pb4php/message/pb_message.php';   // pb4php文件
require_once 'pb_proto_VOA_php_MainLogic.php';   // 自动生成的pb文件

$request = new GetTitlesRequest();
$request->set_type("standard");
$body_str = $request->serializeToString();
$seq = rand();

// 打包示例
// 参数1 -- 业务方法名： string
// 参数2 -- 包体：string   【注意】为protobuf序列化后的二进制字符串
// 参数3 -- 序列号：int     用来判断收发包报文以否一致，防止窜包
// 返回值 -- null：失败  
//           非null：成功  为打包好后的二进制包
$req_pkg = srpc_serialize("MainLogic.MainLogicService.GetTitles", $body_str, $seq);
if ($req_pkg === null)
{
    echo "srpc_pack failed";
    return -1;
}

// 业务收发包: 伪代码
//$rsp_pkg = send_rcv($addr, $req_pkg);
//

$rsp_pkg = "";
$errmsg = "";
$ret = send_rcv("VOA_php.MainLogic", $req_pkg, $rsp_pkg, $errmsg, "srpc_check_pkg", 10000000);
if ($ret != 0)
{
    echo "send_rcv failed ", $ret, $errmsg;
    return -1;
}

echo "send_rcv ", $ret, "\n";
// 解包示例
// 参数     -- 收到的回包：string
// 返回值   -- 返回值为一个数组，包含“errmsg”，“seq”，“body”；
//             都是string类型
// 【注意】 1. 先要判断errmsg是否为success或者Success；
//          2. 再判断seq是否和打包时一致；
//          3. 1，2判断都正确，才能把body当成正确的报文解包；
//  该函数使用示例可参见开发包的call_service.php文件
$ret = srpc_deserialize($rsp_pkg);
if (($ret['errmsg'] !== 'success') && ($ret['errmsg'] !== 'Success'))
{
    echo "srpc_unpack failed ", $ret['errmsg'];
    return -2;
}

echo "seq ", $ret['seq'], " ---- ", $seq, "\n";
if ($ret['seq'] != $seq)
{
    echo "the sequence is inconsistent";
    return -3;
}

$body_str = $ret['body'];

$response = new GetTitlesResponse();
$response->ParseFromString($body_str);

echo "status:".$response->status()."\n";
echo "title number:".$response->titles_size()."\n";
for ($i = 0; $i < $response->titles_size(); ++$i)
{
        echo "title#".$i.":".$response->titles($i)."\n";
}

//var_dump($response);


function send_rcv($servicename,
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

    var_dump($route);

    $comm_type = $route["type"];

    if ($comm_type == "tcp" || $comm_type == "all") {
        return send_rcv_tcp($route,
            $request ,$response, $errmsg, $callback_func, $timeout_usec);
    } else if ($comm_type == "udp") {

        return send_rcv_udp($route,
            $request ,$response, $errmsg, $timeout_usec);
    } else {
        $errmsg = ("unknown comm_type:" . $comm_type);
        return -1;
    }
}

function send_rcv_tcp(array  $route,
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

function send_rcv_udp(array  $route,
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
    socket_bind($fd, "10.104.246.209", 7963);

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
