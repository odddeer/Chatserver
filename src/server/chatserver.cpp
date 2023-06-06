//
// Created by lenovo on 2023/5/24.
//
#include "../../include/server/chatserver.h"
#include "../../thirdparty/json.hpp"
//#include "json.hpp"
#include"../../include/server/chatservice.hpp"

#include<functional>
#include <string>
using namespace std;
using namespace placeholders;
using json=nlohmann::json;

//初始化聊天服务对象（构造函数）
ChatServer::ChatServer(EventLoop* loop,
const InetAddress& listenAddr,
const string& nameArg): _server(loop,listenAddr,nameArg),_loop(loop){
    //注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));

    //注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessege,this,_1,_2,_3));

    //设置线程数量
    _server.setThreadNum(4);//设置线程数，对应cpu
}

//启动服务
void ChatServer::start() {
    _server.start();
}

//上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr& conn){
    //客户端断开链接
    if(!conn->connected()){//客户端断开连接后，调用聊天业务类完成id-socket表映射删除，更新对应用户的数据库状态offline
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();//关闭socket连接下树
    }
}
//上报读写事件相关信息的回调函数
void ChatServer::onMessege(const TcpConnectionPtr& conn,
               Buffer *buffer,
               Timestamp time)
{
     string buf=buffer->retrieveAllAsString();//把buffer接收到的内容转成字符串
    //数据的反序列化 如：string -> 结构体
    json js =json::parse(buf);
    //达到的目的：完全解耦网络模块的代码和业务模块的代码
    //通过js["msgid"]获取-》业务处理器（handler,绑定好的回调函数）-》conn js time传给该回调函数处理业务
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn,js,time);//好处：网络架构根本无需关注业务函数，直接用调用替换，调用函数可以在业务层随机替换
}

