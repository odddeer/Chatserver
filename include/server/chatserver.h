//
// Created by lenovo on 2023/5/24.
//

#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop* loop,               //epoll树
               const InetAddress& listenAddr, //bind的socket结构体
               const string& nameArg);        //服务器取名
    //启动服务
    void start();
private:
    //上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr& conn);
    //上报读写事件相关信息的回调函数
    void onMessege(const TcpConnectionPtr& conn,
                   Buffer*,
                   Timestamp);
    TcpServer _server;// 组合的muduo库，实现服务器功能的类对象
    EventLoop *_loop; //指向事件循环的指针 epoll返回事件数组的指针
};

#endif //QQ_CHATSERVER_H
