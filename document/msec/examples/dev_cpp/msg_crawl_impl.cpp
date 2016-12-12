/**
 * @brief 自动生成的业务代码逻辑实现
 */

#include "syncincl.h"
#include "srpcincl.h"
#include "frozen.h"
#include "msg_crawl_impl.h"
#include "../include/srpcincl.h"
#include <stdio.h>
#include <stdint.h>
#include <cstring>


static int32_t cb(void *buf, int32_t len)
{
	if (len < 10) {return 0;}


	char lenStr[11];

	memcpy(lenStr, buf, 10);
	lenStr[10] = 0;

	int index = 9;
	while ( lenStr[index] == ' ' && index >= 0) { lenStr[index] = '\0'; index--;}

	int jsonLen = atoi(lenStr);
	NGLOG_ERROR("in cb, jsonLen=%d\n", jsonLen);

	if (len >= (jsonLen+10)) { return jsonLen+10;}
	else { return 0;}
}



/**
 * @brief  自动生成的业务方法实现接口
 * @param  request  [入参]业务请求报文
 *         response [出参]业务回复报文
 * @return 框架会将返回值作为执行结果传给客户端
 */
int CCrawlServiceMsg::GetMP3List(const GetMP3ListRequest* request, GetMP3ListResponse* response)
{    
	ATTR_REPORT("GetMP3List_entry");
	if (request->type().length() < 1 || 
		strcmp(request->type().c_str(), "standard") != 0 && strcmp(request->type().c_str(), "special") != 0 )
	{
		response->set_status(100);
		ATTR_REPORT("GetMP3List_EXIT_FAIL");
		response->set_msg("type field invalid");
		return 0;
	}
	char lenStr[11];
	char jsonReq[1024];
	char jsonResp[102400];
	char * respPtr = NULL; 




	snprintf(jsonReq+10, sizeof(jsonReq)-10, 
		"{\"handleClass\":\"com.bison.GetMP3List\",  \"requestBody\": {\"type\":\"%s\"} }",
		request->type().c_str());
	snprintf(lenStr, sizeof(lenStr), "%-10d", strlen(jsonReq+10));
	memcpy(jsonReq, lenStr, 10);


	NGLOG_INFO("json request:%s\n", jsonReq);

	CSrpcProxy proxy("Jsoup.jsoup"); 
	proxy.SetThirdCheckCb(cb);

	int32_t respLen ;
	if ( SRPC_SUCCESS != proxy.CallMethod(jsonReq, (int32_t)strlen(jsonReq), respPtr, respLen, 20000))
	{
		NGLOG_ERROR("call method failed");
		ATTR_REPORT("GetMP3List_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("accessing java json service failed.");
		return 0;
	}

	if (respLen >= sizeof(jsonResp))
	{
		NGLOG_ERROR("respLen too large:%d", respLen);
		ATTR_REPORT("GetMP3List_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("response json string is too long");
		return 0;
	}
	NGLOG_INFO("respLen:%d\n", respLen);
	memcpy(jsonResp, respPtr, respLen);
	jsonResp[respLen] = '\0';
	free(respPtr);

	

	struct json_token * obj, *status, *mp3s;
	obj = parse_json2(jsonResp+10, respLen - 10);    
	if (obj == NULL)    {        
		NGLOG_ERROR("json parse faild"); 
		ATTR_REPORT("GetMP3List_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("json parsing failed");
		goto l_end;    
	}	
	status = find_json_token(obj, "status");	
	if (status == NULL || status->type != JSON_TYPE_NUMBER || status->len > 10)	
	{		
		NGLOG_ERROR("json parse:status field invalid");
		ATTR_REPORT("GetMP3List_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("json parsing failed");
		goto l_end;	
	}    
	int iStatus;
	char sStatus[100];
	memcpy(&sStatus, status->ptr, status->len);
	sStatus[status->len] = '\0';
	iStatus = atoi(sStatus);
	NGLOG_INFO("jsoup service return status=%d", iStatus);
	if (iStatus != 0)
	{
		NGLOG_ERROR("json parse:status field value is no zero");
		ATTR_REPORT("GetMP3List_EXIT_FAIL");
		response->set_status(100);
		response->set_msg("java service returns errcode");
		goto l_end;	
	}
	int i;

	for (i = 0; ; i++)
	{
		char token_name[100];
		snprintf(token_name, sizeof(token_name), "mp3s[%d]", i);
		mp3s = find_json_token(obj, token_name);
		if (mp3s == NULL || mp3s->type != JSON_TYPE_OBJECT)	
		{		
			break;
		}    
		struct json_token * title, *url;

		title = find_json_token(mp3s, "title");
		url = find_json_token(mp3s, "url");
		if (title == NULL || title->type != JSON_TYPE_STRING ||
		 url == NULL || url->type != JSON_TYPE_STRING)
		{
			NGLOG_ERROR("json parse:url or title field invalid");
			ATTR_REPORT("GetMP3List_EXIT_FAIL");
			response->set_status(100);
			response->set_msg("json parsing failed");
			goto l_end;
		}
		char sUrl[1024];
		char sTitle[1024];
		if (url->len >= sizeof(sUrl) || url->len < 0 || 
			title->len >= sizeof(sTitle) || title->len < 0)
		{
			NGLOG_ERROR("json parse:url or title field length invalid");
			ATTR_REPORT("GetMP3List_EXIT_FAIL");
			response->set_status(100);
			response->set_msg("json parsing failed");
			goto l_end;
		}
		memcpy(sUrl, url->ptr, url->len); sUrl[url->len] = '\0';
		memcpy(sTitle, title->ptr, title->len); sTitle[title->len] = '\0';
		NGLOG_INFO("got one mp3:%s,%s\n", sUrl, sTitle);	

		::crawl::OneMP3* one = response->add_mp3s();
		one->set_title(sTitle);
		one->set_url(sUrl);
	}
	NGLOG_INFO("mp3 number:%d", i);

	
	response->set_status(0);
	response->set_msg("success");
	NGLOG_INFO("success");
	ATTR_REPORT("GetMP3List_EXIT_SUC");
l_end:	
	if (obj != NULL) { free(obj);}		

	
        return 0;
}



