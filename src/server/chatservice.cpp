//
// Created by lenovo on 2023/5/24.
//
#include "../../include/server/chatservice.hpp"
#include "../../include/public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
#include<map>
using namespace std;
using namespace muduo;


//获取单例对象的接口函数
ChatService* ChatService::instance() {
    static ChatService service;
    return &service;
}

//注册消息以及对应的回调操作
ChatService::ChatService(){
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});

    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
}

//服务器异常，业务重置方法
void ChatService::reset(){
    //把online状态的用户，设置成offline
    _userModel.resetState();
}

//注册消息以及对应的Handler回调操作
MsgHandler ChatService::getHandler(int msgid) {
    //记录错误日志，msgid没有对应的事件处理回调
    auto it=_msgHandlerMap.find(msgid);
    if(it==_msgHandlerMap.end()){//map中找不到对应msgid,调用muduo打印错误日志
        //返回错误日志，msgid没有对应的事件处理回调
               //[=]表示值传递方式捕获所有父作用域的变量（包括this）
        return [=](const TcpConnectionPtr &conn,json &js,Timestamp){
            LOG_ERROR << "msgid:"<<msgid<<" can not find handler!";
        };
    }
    else{//找到msgid，返回对应回调function函数对象
        return _msgHandlerMap[msgid];
    }
}

//处理登录业务  js只要输入 name password即可登录
void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time){
    int id =js["id"].get<int>();//js出来都是字符串，要转整型
    string pwd=js["password"];

    User user=_userModel.query(id);//根据id查询消息，判断回复什么
    if(user.getId()==id && user.getPwd()==pwd)
    {//不是默认user对象id(-1)，且密码正确判断是否重复登录
        if(user.getState()=="online")
        {
            //该用户已经登录，不允许重复登录
            json response;
            response["msgid"]=LOGIN_MSG_ACK;//消息类别：
            response["errno"]=2;//与客户端指定，2出错，代表重复登录
            response["errmsg"]="this account is using, input another!";//该账号已经登录，请重新输入新账号
            conn->send(response.dump());//socket发出
        }
        else
        {
            //登录成功，记录用户连接信息 (id 和 socket)
            //考虑map安全，加入智能指针锁,用完释放
            {//锁，作用域
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }

            //登录成功，更新用户状态信息 state offline=>online
            user.setState("online");
            _userModel.updateState(user);
            json response;
            response["msgid"]=LOGIN_MSG_ACK;//消息类别：回应类消息
            response["errno"]=0;//与客户端指定，0成功
            response["id"]=user.getId();
            response["name"]=user.getName();
            //查询该用户是否有离线消息 (offlinemsg业务)
            vector<string> vec=_offlineMsgModel.query(id);
            if(!vec.empty()){
                response["offlinemsg"]=vec;//直接添加序列化
                //读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            //查询该用户的好友信息
            vector<User> userVec=_friendModel.query(id);
            if(!userVec.empty()){
                vector<string> vec2;
                for(User &user:userVec){
                    json js;
                    js["id"]=user.getId();
                    js["name"]=user.getName();
                    js["state"]=user.getState();
                    vec2.push_back(js.dump());//改成js字符串后压回vec2中
                }
                response["friends"]=vec2;
            }
            //查询用户的群组信息 (群消息+群成员信息)
            vector<Group> groupuserVec=_groupModel.queryGroups(id);
            if(!groupuserVec.empty())
            {
                //group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for(Group &group:groupuserVec)
                {
                    json grpjson;
                    grpjson["id"]=group.getId();
                    grpjson["groupname"]=group.getName();
                    grpjson["groupdesc"]=group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user:group.getUsers())
                    {
                        json js;
                        js["id"]=user.getId();
                        js["name"]=user.getName();
                        js["state"]=user.getState();
                        js["role"]=user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"]=userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"]=groupV;
            }
            printf("%s\n",response.dump().c_str());
            conn->send(response.dump());//socket发出序列化内容
        }
    }
    else{
        //该用户不存在，登录失败
        json response;
        response["msgid"]=LOGIN_MSG_ACK;//消息类别：
        response["errno"]=1;//与客户端指定，1出错,代表没搜到
        response["errmsg"]="id or password is invalid!";
        conn->send(response.dump());//socket发出
    }
}

//处理注册业务  js只要输入name password即可注册
void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time){
    string name=js["name"];
    string pwd=js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state=_userModel.insert(user);
    if(state){
        //注册成功
        json response;
        response["msgid"]=REG_MSG_ACK;//消息类别：回应类消息
        response["errno"]=0;//与客户端指定，0成功
        response["id"]=user.getId();
        conn->send(response.dump());//socket发出
    }
    else{
        //注册失败
        json response;
        response["msgid"]=REG_MSG_ACK;//消息类别：
        response["errno"]=1;//与客户端指定，1出错
        conn->send(response.dump());//socket发出
    }
}

//处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it=_userConnMap.find(userid);
        if(it!=_userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    //更新用户的状态信息
    User user(userid,"","","offline");
    _userModel.updateState(user);//预数据库连接的业务对象更新

}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn){
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it) {
            if (it->second == conn) {
                //从map表删除用户的链接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    //更新用户的状态信息
    if(user.getId()!=-1){//不是空用户
        user.setState("offline");//与数据库连接的对象状态设置
        _userModel.updateState(user);//预数据库连接的业务对象更新
    }
}

//获取消息对应map回调函数的处理器
void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time){
    int toid=js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it=_userConnMap.find(toid);
        if(it!=_userConnMap.end()){//找到
            //toid在线，转发消息  服务器把来源socket的消息js中转给目标客户
            it->second->send(js.dump());
            return;
        }
    }
    //toid不在线，存储离线消息
    _offlineMsgModel.insert(toid,js.dump());
}

//添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time){
    int userid=js["id"].get<int>();//把申请加别人好友的人的id转数字
    int friendid=js["friendid"].get<int>();//把申请目标好友id转数字

    //存储好友信息
    _friendModel.insert(userid,friendid);
}


//群组相关业务：
//创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    //从js中获得相关创建组信息
    int userid = js["id"].get<int>();//获得创建群组用户号
    string name=js["groupname"];
    string desc=js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1,name,desc);
    if(_groupModel.createGroup(group)){//调用_groupModel对象在allgroup表加入创建群
        //存储群组创建人信息                                                                     （人，组，角色）
        _groupModel.addGroup(userid,group.getId(),"creator");//顺手调用_groupModel对象在groupuser表加入”创建者“角色
    }
}
//加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time) {
    //从js中获得加入组人员信息，及目标加入组，顺手调用_groupModel对象在groupuser表加入”普通“角色
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}
//群组聊天业务（是群消息，发给群里每个人对应（map）的socket,在线的直接发，不在线的存offlinemessage表中，登录再发）
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    vector<int> useridVec=_groupModel.queryGroupUsers(userid,groupid);//找出某个群除自己以外所有人id
    lock_guard<mutex> lock(_connMutex);
    for(int id:useridVec){
      auto it = _userConnMap.find(id);//加锁保证公共资源map表安全
      if(it!=_userConnMap.end()){
         //转发群消息
         it->second->send(js.dump());
      }
      else{

          //存储离线群消息
          _offlineMsgModel.insert(id,js.dump());//存到offlinemessage表中，id,离线消息
      }
    }
}

