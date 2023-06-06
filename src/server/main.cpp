//
// Created by lenovo on 2023/5/24.
//
#include "../../include/server/chatserver.h"
#include "../../include/server/chatservice.hpp"
#include<iostream>
#include<signal.h>
using namespace std;

//处理服务器ctrl+c结束后，重载user的状态信息
void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}

int main(){
    signal(SIGINT,resetHandler);//ctrl+c

    EventLoop loop;// 类似epoll创建
    InetAddress addr("127.0.0.1",6000);//定义socket结构体的ip和port
    ChatServer server(&loop,addr,"ChatServer");//bind

    server.start();// listen epoll_ctl添加到epoll上
    loop.loop();// epoll_wait以阻塞的方式等待新用户连接，已连接用户的读写事件等，然后回调onConnection和onMessage方法
    return 0;
}
