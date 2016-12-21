
# -*- coding: utf-8 -*-
import msec_pb2
import json
from msec_impl import *
import VOA_py_Crawl_pb2
import protobuf_json

#
#  @brief 自动生成的业务代码逻辑实现
#  Python版本实现部分
#

class MainLogicService:

    # @brief  自动生成的业务方法实现接口
    # @param  request  [入参]业务请求报文，非pb格式，需要转换成pb
    # @return 业务回复报文，pb序列化后的报体
    def GetTitles(self, req_data, is_json):

        # 自动生成部分，反序列化请求包体
        request = msec_pb2.GetTitlesRequest()
        response = msec_pb2.GetTitlesResponse()

        # json协议处理
        if is_json:
            req_json = json.loads(req_data)
            request = protobuf_json.json2pb(request, req_json)
        else:
            request.ParseFromString(req_data)

        # TODO: 业务逻辑实现
        level = get_config('LOG', 'Level')
        if level:
            log_info(level)
        level = get_config('LOG', 'level')
        if level:
            log_info(level)

        try:
            import MySQLdb  #mysql
        except ImportError:
            log_error("Fail to import mysql library.")
        else:
            route = getroute("Database.mysql")
            if route:
                # 打开数据库连接
                db = MySQLdb.connect(route["ip"], "msec", "msec@anyhost", "msec_test_java_db", route["port"])
                cursor = db.cursor()
                cursor.execute("SELECT VERSION()")
                data = cursor.fetchone()
                log_info("Database version : %s" % data)
                db.close()

        crawlreq = VOA_py_Crawl_pb2.GetMP3ListRequest()
        crawlreq.type = request.type
        # result = CallMethod("VOA_java.Crawl", "crawl.CrawlService.getMP3List", crawlreq, 20000)
        result = CallMethod("VOA_py.Crawl", "crawl.CrawlService.GetMP3List", crawlreq, 20000)
        if result["ret"] != 0:
            response.status = 100
            response.msg = "CallMethod failed: "+result["errmsg"]
            log_error('callmethod error: %d %s' % (result["ret"], result["errmsg"]))
        else:
            crawlrsp = VOA_py_Crawl_pb2.GetMP3ListResponse()
            crawlrsp.ParseFromString(result["response"])
            if crawlrsp.status != 0:
                log_error('getmp3list response error: %d %s' % (crawlrsp.status, crawlrsp.msg))
                response.status = 100
                response.msg = "getmp3list response failed: " + crawlrsp.msg
            else:
                response.status = 0
                response.msg = "success"
                for mp3 in crawlrsp.mp3s:
                    response.titles.append(mp3.title)
                log_info( 'GetTitles Success')
                attr_report('GetTitles success')

        # 序列化回复包体
        if is_json:
            return json.dumps(protobuf_json.pb2json(response))
        else:
            return response.SerializeToString()

    
    # @brief  自动生成的业务方法实现接口
    # @param  request  [入参]业务请求报文，非pb格式，需要转换成pb
    # @return 业务回复报文，pb序列化后的报体
    def GetUrlByTitle(self, req_data, is_json):

        # 自动生成部分，反序列化请求包体
        request = msec_pb2.GetUrlByTitleRequest()
        response = msec_pb2.GetUrlByTitleResponse()
        # json协议处理
        if is_json:
            req_json = json.loads(req_data)
            request = protobuf_json.json2pb(request, req_json)
        else:
            request.ParseFromString(req_data)

        # TODO: 业务逻辑实现
        log_info("GetUrlByTitle start....")
        attr_report("GetUrlByTitle Entry")
        crawlreq = VOA_py_Crawl_pb2.GetMP3ListRequest()
        crawlreq.type = request.type
        # result = CallMethod("VOA_java.Crawl", "crawl.CrawlService.getMP3List", crawlreq, 20000)
        result = CallMethod("VOA_py.Crawl", "crawl.CrawlService.GetMP3List", crawlreq, 20000)
        if result["ret"] != 0:
            response.status = 100
            response.msg = "CallMethod failed: " + result["errmsg"]
            log_error('callmethod error: %d %s' % (result["ret"], result["errmsg"]))
        else:
            crawlrsp = VOA_py_Crawl_pb2.GetMP3ListResponse()
            crawlrsp.ParseFromString(result["response"])
            if crawlrsp.status != 0:
                log_error('getmp3list response error: %d %s' % (crawlrsp.status, crawlrsp.msg))
                response.status = 100
                response.msg = "getmp3list response failed: " + crawlrsp.msg
            else:
                response.status = 100
                response.msg = "failed to find the url"
                for mp3 in crawlrsp.mp3s:
                    if request.title == mp3.title:
                        response.url = mp3.url
                        response.status = 0
                        response.msg = "success"
                        log_info("GetUrlByTitle succ.")
                        attr_report("GetUrlByTitle succ")
                        break

        # 序列化回复包体
        if is_json:
            return json.dumps(protobuf_json.pb2json(response))
        else:
            return response.SerializeToString()
