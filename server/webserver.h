#ifndef WEBSERVER_H
#define WEBSERVER_H

#include<unordered_map>
#include<fcntl.h>//fcntl()
#include<unistd.h> //close()
#include<assert.h>
#include<errno.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include"epoller.h"
#include"../timer/heaptimer.h"
#include"../log/log.h"
#include"../pool/sqlconnpool.h"
#include"../pool/threadpool.h"
#include"../http/httpconn.h"

class WebServer{
public:
    WebServer(
        int port,int trigMode,int timeoutMS,
        int sqlPort,const char* sqlUser, const char* sqlPwd,
        const char* dbName, int connPoolNum, int threadNum,
        bool openLog,int logLevel, int logQueSize
    );
    ~WebServer();
    void Start();

private:
    bool InitSocket_();
    void InitEvenMode_(int trigMode);
    void AddClient_();

    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd,const char* info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess_(HttpConn* client);

    static const int MAX_FD = 65536;
    static int SetFdNonblock(int fd);

    int port_;
    bool openLinger_;
    int timeoutMS_;//毫秒
    bool isClose_;
    int listenFd_;
    char* srcDir_;//文件路径

    //网络编程中常使用32位整数表示事件掩码或标志位
    uint32_t listenEvent_;//监听事件
    uint32_t connEvent_;//连接事件

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int,HttpConn> user_;
};

#endif

#endif