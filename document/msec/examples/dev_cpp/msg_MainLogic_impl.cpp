/**
 * @brief 自动生成的业务代码逻辑实现
 */

#include "syncincl.h"
#include "srpcincl.h"
#include "msg_MainLogic_impl.h"
#include "VOA_cpp_Crawl.pb.h"
#include "../include/sppincl.h"
#include "../include/srpcincl.h"

#include "../include/mysqlclient/mysql.h"


extern "C"
{
struct routeid {
	uint32_t ip;
	uint16_t port;
	int type;
};

int32_t getroutebyname(const char *name, struct routeid *);

};
static void AccessMysql(::crawl::GetMP3ListResponse  & resp)
{
	if (resp.mp3s_size() < 1)
	{
		return;
	}
	MYSQL * my = mysql_init(NULL);
	if (my == NULL)
	{
		NGLOG_ERROR("mysql_init faild!\n");
		return;
	}

	struct routeid r;
	if (0 != getroutebyname("Database.mysql", &r))
	{
		NGLOG_ERROR("getroutebyname() failed!\n");
		return;
	}
	struct in_addr addr;
	addr.s_addr = r.ip;
	char * ip = inet_ntoa(addr);
	if (ip != NULL)
	{
		NGLOG_INFO("mysql server ip:%s\n", ip);
	}
	char host[100] = "127.0.0.1";
	strcpy(host, ip);
	char user[100]="msec";
	char passwd[100] = "msec@anyhost";
	char db[100] = "msec_test_db";
	if ( NULL == mysql_real_connect(my, host, user, passwd, db, 3306, NULL, 0))
	{
		NGLOG_ERROR("mysql connect failed!%s\n", mysql_error(my));
		mysql_close(my);
		return;
	}
	char sqlStr[1024];
	mysql_query(my, "delete from mp3_list");
	snprintf(sqlStr, sizeof(sqlStr), "insert into mp3_list values(?,?)");
	MYSQL_STMT * stmt = mysql_stmt_init(my);
	if (stmt == NULL)
	{
		NGLOG_ERROR("mysql stmt init failed! %s\n", mysql_error(my));
		mysql_close(my);
		return;
	}
	if (0 != mysql_stmt_prepare(stmt, sqlStr, strlen(sqlStr)))
	{
		NGLOG_ERROR("mysql stmt prepare failed! %s\n", mysql_error(my));
		mysql_stmt_close(stmt);
		mysql_close(my);
		return;
	}
	for (uint32_t i = 0; i < resp.mp3s_size(); ++i)
	{
		MYSQL_BIND bind[2];
		memset(&bind, 0, sizeof(bind));

		bind[0].buffer_type = MYSQL_TYPE_STRING;
		bind[0].buffer = (void *) resp.mp3s(i).title().c_str();
		bind[0].buffer_length = resp.mp3s(i).title().length();
		bind[0].is_null = 0;


		bind[1].buffer_type = MYSQL_TYPE_STRING;
		bind[1].buffer = (void *) resp.mp3s(i).url().c_str();
		bind[1].buffer_length = resp.mp3s(i).url().length();
		bind[1].is_null = 0;


		if (0 != mysql_stmt_bind_param(stmt, bind))
		{
			NGLOG_ERROR("mysql stmt bind failed! %s\n", mysql_error(my));
			mysql_stmt_close(stmt);
			mysql_close(my);
			return;
		}
		if (0 != mysql_stmt_execute(stmt))
		{
			NGLOG_ERROR("mysql stmt exec failed! %s\n", mysql_error(my));
			mysql_stmt_close(stmt);
			mysql_close(my);
			return;
		}
		NGLOG_INFO("replace into mysql success #%u", i);

	}
	mysql_stmt_close(stmt);
	mysql_close(my);


}












/**
 * @brief  自动生成的业务方法实现接�?
 * @param  request  [入参]业务请求报文
 *         response [出参]业务回复报文
 * @return 框架会将返回值作为执行结果传给客户端
 */
int CMainLogicServiceMsg::GetTitles(const GetTitlesRequest* request, GetTitlesResponse* response)
{    

	::crawl::GetMP3ListRequest req;
	::crawl::GetMP3ListResponse resp;

	ATTR_REPORT("GetTitles_entry");

	req.set_type(request->type());

	int ret = CallMethod("VOA_cpp.Crawl", "crawl.CrawlService.GetMP3List", req, resp, 20000);
	//int ret = CallMethod("VOA_java.Crawl", "crawl.CrawlService.getMP3List", req, resp, 20000);
	if (ret != SRPC_SUCCESS)
	{
    		NGLOG_ERROR("call method failed:%d\n", ret);
		ATTR_REPORT("GetTitles_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("call method communication failed!");
		return 0;
	}
	NGLOG_INFO("crawl.CrawlService.GetMP3List communicates success\n");
	if (resp.status() != 0)
	{
    		NGLOG_ERROR("GetMP3List failed:%d, %s\n", resp.status(), resp.msg().c_str());
		ATTR_REPORT("GetTitles_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("GetMP3List() failed!");
		return 0;
	}
	NGLOG_DEBUG("crawl.CrawlService.GetMP3List success, mp3 number:%d\n", resp.mp3s_size());

	int i;
	for (i = 0; i < resp.mp3s_size(); ++i)
	{
		::crawl::OneMP3 one = resp.mp3s(i);
		response->add_titles(one.title());
		NGLOG_INFO("I got one title:%s\n", one.title().c_str());

	}
	AccessMysql(resp);
	
	NGLOG_INFO("GetTitles success");
	ATTR_REPORT("GetTitles_EXIT_SUC");
	response->set_status(0);
	response->set_msg("success");
        return 0;
}

/**
 * @brief  自动生成的业务方法实现接�?
 * @param  request  [入参]业务请求报文
 *         response [出参]业务回复报文
 * @return 框架会将返回值作为执行结果传给客户端
 */
int CMainLogicServiceMsg::GetUrlByTitle(const GetUrlByTitleRequest* request, GetUrlByTitleResponse* response)
{    
    /**
     * TODO 业务逻辑实现，request/response为业务业务定义的protobuf协议格式
     *      业务可使用框架自带的监控系统 MONITOR("test"), 详见monitor.h
     *      业务可使用框架自带的日志系统 NGLOG_DEBUG("test")，详见srpc_log.h
     */
	::crawl::GetMP3ListRequest req;
	::crawl::GetMP3ListResponse resp;

	ATTR_REPORT("GetUrlByTitle_ENTRY");

	req.set_type(request->type());

	int ret = CallMethod("VOA_cpp.Crawl", "crawl.CrawlService.GetMP3List", req, resp, 20000, PORT_TYPE_TCP);
	if (ret != SRPC_SUCCESS)
	{
    		NGLOG_ERROR("call method failed:%d\n", ret);
		ATTR_REPORT("GetUrlByTitle_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("call method communication failed!");
		return 0;
	}
	NGLOG_INFO("crawl.CrawlService.GetMP3List communicates success\n");
	if (resp.status() != 0)
	{
    		NGLOG_ERROR("GetMP3List failed:%d, %s\n", resp.status(), resp.msg().c_str());
		ATTR_REPORT("GetUrlByTitle_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("GetMP3List() failed!");
		return 0;
	}
	NGLOG_INFO("crawl.CrawlService.GetMP3List success, mp3 number:%d\n", resp.mp3s_size());

	int i;
	for (i = 0; i < resp.mp3s_size(); ++i)
	{
		::crawl::OneMP3 one = resp.mp3s(i);
		if (one.title().compare(request->title()) == 0)
		{
			NGLOG_DEBUG("Got the url for title %s\n", request->title().c_str());
			response->set_url(one.url());
			break;
		}

        }
	if (!response->has_url())
	{
		ATTR_REPORT("GetUrlByTitle_EXIT_FAIL");
    		NGLOG_ERROR("failed to find url for %s\n", request->title().c_str());
		response->set_status(100);
		response->set_msg("failed to find url for this title.");
		return 0;

	}
	ATTR_REPORT("GetUrlByTitle_EXIT_SUC");
	response->set_status(0);
	response->set_msg("success");
        return 0;

}






int CMainLogicServiceMsg::DownloadMP3(const DownloadMP3Request* request, DownloadMP3Response* response)
{
	
	
	ATTR