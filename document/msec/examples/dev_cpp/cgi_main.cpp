#include <iostream>
#include "srpc_intf.h"
#include "VOA_cpp_MainLogic.pb.h"
#include "srpc_comm.h"

using namespace std;
using namespace srpc;

//#define SRPC_SUCCESS (0)

int main() {

    srand(getpid());

    CSrpcProxy proxy("");
    proxy.SetCaller("cgi");//调用者身份，用于日志展现
    proxy.SetMethod("MainLogic.MainLogicService.GetTitles");//调用的RPC方法
    unsigned long seq = random();//请求的唯一标识，用于校验应答包，防止串包


    MainLogic::GetTitlesRequest req;//RPC接口的请求
    MainLogic::GetTitlesResponse resp;//RPC接口的应答

    req.set_type("special");//请求包的参数初始化


	//将请求包序列化为字节流，可通过网络发送。注意：序列化后的结果加上了SRPC的内部报文头
    char * pkg = NULL;
    int len ;
    if (proxy.Serialize(pkg, len, seq, req) != SRPC_SUCCESS)
    {
        fprintf(stderr, "failed to serialize,%s\n", proxy.GetErrText());
        return -1;
    }
    printf("serialize successfully, len=%d\n", len);

	//网络通信的基本工作
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        free(pkg);
        return -1;
    }
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(); // 服务器IP填入
    serv_addr.sin_port = htons(7963);
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        free(pkg);
        return -1;
    }
    printf("connect to server successfully\n");

	//发送srpc请求
    if (send(sockfd, pkg, len, 0)!= len)
    {
        perror("send:");
        free(pkg);
        return -1;
    }
    printf("send to server successfully\n");

    free(pkg);//释放Serialize分配的空间
	
	//流式通信情况下，反复收应答数据，直到获得一个完整的应答报文
    int offset = 0;
    static char buffer[1024*1024];
    int respLen = 0;
    while (1)
    {
        int recvLen = recv(sockfd, buffer+offset, sizeof(buffer)-offset, 0);
        if (recvLen < 0)
        {
            perror("recv:");
            return -1;
        }
        printf("recv get %d bytes\n", recvLen);
        offset += recvLen;

        respLen = proxy.CheckPkgLen(buffer, offset);//检查报文完整性
        if (respLen < 0)//出错了
        {
            fprintf(stderr, "package format invalid!%s\n", proxy.GetErrText());
            return -1;
        }
        if (respLen > 0)//已经收到了一个完整的报文，长度为respLen
        {
            break;
        }
		if (respLen == 0)//还没有收完，需要继续收
		{
			continue;
		}
    }
    close(sockfd);
    printf("get %d bytes totally\n", respLen);

	unsigned long seq_in_resp;
	//解析报文
    if (SRPC_SUCCESS != proxy.DeSerialize(buffer, respLen, resp, seq_in_resp))
    {
        fprintf(stderr, "DeSerialize failed!%s\n", proxy.GetErrText());
        return -1;
    }
    if (seq_in_resp != seq)//sequence一致吗？
    {
        fprintf(stderr, "sequence mismatch!\n");
        return -1;
    }

	//业务层面的返回码、业务数据等
    printf("server response:%d %s\n", resp.status(), resp.msg().c_str());
    printf("titles number:%d\n", resp.titles_size());
    int i;
    for (i = 0; i < resp.titles_size(); ++i)
    {
        printf("title#%d:%s\n", i,
            resp.titles(i).c_str());
    }



    return 0;
}