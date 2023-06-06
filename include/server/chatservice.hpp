//
// Created by lenovo on 2023/5/24.
//
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<muduo/net/TcpConnection.h>
#include <unordered_map>
#include<functional>
#include<mutex> //互斥锁头文件
using namespace std;
using namespace muduo::net;
using namespace muduo;

#include "model/groupmodel.hpp" //组相关业务
#include "../../thirdparty/json.hpp" //json库方便使用
#include "model/usermodel.hpp"//user表的使用
#include "model/offlinemessagemodel.hpp"//offlinemsg表的使用
#include"model/friendmodel.hpp"//friend表的使用
using json=nlohmann::json;

using MsgHandler=std::function<void(const TcpConnectionPtr &conn,json &js, Timestamp)>;

//聊天服务器业务类
class ChatService{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理登录业务
    void login(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //一对一聊天业务（服务器中转消息）
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time);
    //获取消息对应map回调函数的处理器
    MsgHandler getHandler(int msgid);
    //服务器异常，业务重置方法
    void reset();

    //群组相关业务：
    //创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
private:
    //聊天服务器构造函数
    ChatService();
    //存储消息id和其对应的处理方法 map映射
    unordered_map<int,MsgHandler> _msgHandlerMap;//用map完成序号和回调函数function对象的映射

    //存储在线用户的通信连接 通信有来有望，要把id与socket映射起来，在一个socket里存来往信息
    unordered_map<int,TcpConnectionPtr> _userConnMap;//多用户访问登录，要注意线程安全

    //定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    //user登录表数据操作类对象
    UserModel _userModel;//user表的业务类

    //offline消息操作类对象
    OfflineMsgModel _offlineMsgModel;
    //friend表操作类对象
    FriendModel _friendModel;
    //群组相关业务操作类对象
    GroupModel _groupModel;
};

#endif
