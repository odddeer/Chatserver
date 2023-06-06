//
// Created by hp-pc on 2023/5/30.
//
#include "../../thirdparty/json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
using namespace std;
using json=nlohmann::json;

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<semaphore.h> //linux信号量
#include<atomic> //原子类c++11

// 与服务端共用的头文件
#include "../../include/server/model/group.hpp"
#include "../../include/server/model/user.hpp"
#include "../../include/public.hpp"

//记录服务器发来的信息，不用客户端用的时候再找服务端要，客户端本地备份查找效率高
//记录当前客户端登录的使用者信息
User g_currentUser;
//记录当前登录者的好友列表信息
vector<User> g_currentUserFriendList;
//记录当前登录者的群组列表信息
vector<Group> g_currentUserGroupList;

//控制聊天页面程序
bool isMainMenuRunning= false;

//用于读写线程之间的通信
sem_t rwsem;
//记录登录状态
atomic_bool g_isLoginSuccess(false);

//接收线程声明
void readTaskHandler(int clientfd);
//获取系统时间声明（聊天信息需要添加时间信息）
string getCurrentTime();
//主聊天页面程序声明
void mainMenu(int clientfd);
//显示当前登录成功用户的基本信息函数声明
void showCurrentUserData();

// 聊天客户端程序实现， main线程用作发送线程，子线程用作接收线程（socket并行收发，所以设置两个线程）
int main(int argc, char **argv){
    if(argc<3){
        cerr<<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip=argv[1];
    uint16_t port=atoi(argv[2]);

    //创建client端的socket
    int clientfd=socket(AF_INET,SOCK_STREAM,0);//ipv4 tcp
    if(-1==clientfd){
        cerr<<"socket create error"<<endl;
        exit(-1);
    }

    //填写client需要连接的server信息ip+port   即连接目标套接字的结构体
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr_in));

    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr= inet_addr(ip);

    // client和server进行连接
    if(-1==connect(clientfd,(sockaddr*)&server,sizeof(sockaddr_in))){
        cerr<<"connect server error"<<endl;
        close(clientfd);
        exit(-1);
    }

    //初始化读写线程通信用的信号量
    //             进程还是线程（0）通信
    sem_init(&rwsem,0,0);

    //连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler,clientfd);
    readTask.detach();

    // main线程用于接收用户输入，负责发送数据
    for(;;){
        //显示首页面菜单 登录、注册、退出
        cout<<"====================="<<endl;
        cout<<"1. login"<<endl;
        cout<<"2. register"<<endl;
        cout<<"3. quit"<<endl;
        cout<<"====================="<<endl;
        cout<<"choice:"<<endl;
        int choice=0;
        cin>>choice;
        cin.get();//读掉缓冲区残留的回车

        switch (choice) {
            case 1://login 业务
            {
                int id=0;
                char pwd[50]={0};
                cout<<"userid:";
                cin>>id;
                cin.get();
                cout<<"userpassword:";
                cin.getline(pwd,50);

                json js;
                js["msgid"]=LOGIN_MSG;
                js["id"]=id;
                js["password"]=pwd;
                string request=js.dump();

                g_isLoginSuccess=false;

                int len=send(clientfd,request.c_str(), strlen(request.c_str())+1,0);
                //cout<<request.c_str()<<endl;
                if(len==-1){//登录信息发送失败
                    cerr<<"send login msg error:"<<request<<endl;
                }/*
                else
                {//发送完，等回复消息

                    char buffer[1024]={0};cout<<"****"<<endl;
                    len=recv(clientfd,buffer,1024,0);
                    cout<<"xxxx"<<endl;
                    if(-1==len){//接收失败
                        cerr<<"recv login response error"<<endl;
                    }
                    else
                    {//接收到东西
                        json responsejs=json::parse(buffer);
                        if(0!=responsejs["errno"].get<int>()){//回复登录失败
                            cerr<<responsejs["errmsg"]<<endl;
                        }
                        else{//回复登录成功
                            //记录当前用户的id和name
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);

                            //记录当前用户的好友列表
                            if(responsejs.contains("friends")){//看返回的有无朋友列表
                                vector<string> vec=responsejs["friends"];
                                for(string &str:vec){
                                    json js=json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            //记录当前用户的群组列表信息
                            if(responsejs.contains("groups")){
                                vector<string> vec1 = responsejs["groups"];
                                for(string &groupstr:vec1){
                                    json grpjs=json::parse(groupstr);
                                    Group group;
                                    group.setId(grpjs["id"].get<int>());
                                    group.setName(grpjs["groupname"]);
                                    group.setDesc(grpjs["groupdesc"]);

                                    vector<string> vec2=grpjs["users"];
                                    for(string &userstr:vec2)
                                    {
                                        GroupUser user;
                                        json js=json::parse(userstr);
                                        user.setId(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setState(js["state"]);
                                        user.setRole(js["role"]);
                                        group.getUsers().push_back(user);
                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }

                            //显示登录用户的基本信息
                            showCurrentUserData();

                            //显示当前用户的离线消息 个人聊天消息或者群组消息
                            if(responsejs.contains("offlinemsg"))
                            {
                                vector<string> vec=responsejs["offlinemsg"];
                                for(string &str:vec)
                                {
                                    json js=json::parse(str);
                                    //time + [id] + name + "said:" + xxx
                                    if(ONE_CHAT_MSG == js["msgid"].get<int>()){//打印onechat信息收到的一对一离线消息
                                        cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()
                                            <<" said:"<<js["msg"].get<string>()<<endl;
                                    }
                                    else{//群聊离线消息
                                        cout<<"群消息["<<js["groupid"]<<"]:"<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()
                                            <<" said: "<<js["msg"].get<string>()<<endl;
                                    }
                                }
                            }
                            /*
                            //登录成功，启动接收线程负责接收数据；
                            std::thread readTask(readTaskHandler, clientfd);//实际调用的linux的c的pthread_create
                            readTask.detach();//pthread_detach
                             */
                sem_wait(&rwsem);//等待信号量 由子线程监听完登录的响应消息后，通知这里，不唤醒就阻塞
                if(g_isLoginSuccess)
                {
                    //进入聊天主菜单页面
                    isMainMenuRunning=true;
                    mainMenu(clientfd);
                }
                           /*
                            //进入聊天主菜单页面
                            isMainMenuRunning=true;
                            mainMenu(clientfd);
                            */
                        /*}
                    }

                }*/
            }
            break;
            case 2:{//register业务
                char name[50]={0};
                char pwd[50]={0};
                cout<< "username:";
                cin.getline(name,50);
                cout<< "userpassword:";
                cin.getline(pwd,50);

                json js;
                js["msgid"]=REG_MSG;
                js["name"]=name;
                js["password"]=pwd;
                string request = js.dump();

                int len=send(clientfd,request.c_str(), strlen(request.c_str())+1,0);
                if(len==-1){
                    cerr<<"send reg msg error:"<<request<<endl;
                }
                sem_wait(&rwsem);//等待信号量 子线程处理完注册消息通知
                /*
                else{
                    char buffer[1024]={0};
                    cout<<"阻塞等待。。。"<<endl;
                    len=recv(clientfd,buffer,1024,0);//阻塞等待
                    cout<<len<<endl;
                    if(-1==len){//返回信息错误
                        cerr<<"recv reg response error"<<endl;
                    }
                    else{
                        json responsejs = json::parse(buffer);//string->js
                        if(0!=responsejs["errno"].get<int>()){//注册失败，返回注册失败错误码
                            cerr<<name<<" is already exist, register error!"<<endl;
                        }
                        else{//注册成功，返回注册成功及相关信息
                            cout<<name<<" register success, userid is "<<responsejs["id"]
                                <<", do not forget it!"<<endl;
                        }
                    }
                }*/
            }
            break;
            case 3: {//退出，关闭客户端fd,直接退出
                close(clientfd);
                sem_destroy(&rwsem);//原子变量释放
                exit(0);
            }
            default:
                cerr<<"invalid input!"<<endl;
                break;

        }
    }

    return 0;
}


//显示当前登录成功用户的基本信息函数实现
void showCurrentUserData(){
    cout<<"===========login user============"<<endl;
    cout<<"current login user => id:"<<g_currentUser.getId()<<" name:"<<g_currentUser.getName()<<endl;
    cout<<"-----------friend list----------"<<endl;
    if(!g_currentUserFriendList.empty()){//朋友列表不为空,打印朋友列表信息
        for(User &user:g_currentUserFriendList){
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"-----------group list----------"<<endl;
    if(!g_currentUserGroupList.empty()){//组表不为空，打印当前登录者群组，即群组中人员（user+角色）信息
        for(Group &group:g_currentUserGroupList)
        {
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &user:group.getUsers())
            {
                cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<" "
                <<user.getRole()<<endl;
            }
        }
    }
    cout<<"================================"<<endl;
}

//处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{

    if(0!=responsejs["errno"].get<int>()){//回复登录失败
        cerr<<responsejs["errmsg"]<<endl;
        g_isLoginSuccess=false;
    }
    else{//回复登录成功
        //记录当前用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        //记录当前用户的好友列表
        if(responsejs.contains("friends")){//看返回的有无朋友列表
            // 初始化 当用户loginout退出再登录回来，这个全局vector已经存了上一次登录者的朋友消息，这一次再登录再次加入朋友信息重复
            g_currentUserFriendList.clear();//初始化防止朋友信息重复或多加

            vector<string> vec=responsejs["friends"];
            for(string &str:vec){
                json js=json::parse(str);
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        //记录当前用户的群组列表信息
        if(responsejs.contains("groups")){
            // 初始化
            g_currentUserGroupList.clear();//初始化防止群组信息重复或多加

            vector<string> vec1 = responsejs["groups"];
            for(string &groupstr:vec1){
                json grpjs=json::parse(groupstr);
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);

                vector<string> vec2=grpjs["users"];
                for(string &userstr:vec2)
                {
                    GroupUser user;
                    json js=json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }
        }

        //显示登录用户的基本信息
        showCurrentUserData();

        //显示当前用户的离线消息 个人聊天消息或者群组消息
        if(responsejs.contains("offlinemsg"))
        {
            vector<string> vec=responsejs["offlinemsg"];
            for(string &str:vec)
            {
                json js=json::parse(str);
                //time + [id] + name + "said:" + xxx
                if(ONE_CHAT_MSG == js["msgid"].get<int>()){//打印onechat信息收到的一对一离线消息
                    cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()
                        <<" said:"<<js["msg"].get<string>()<<endl;
                }
                else{//群聊离线消息
                    cout<<"群消息["<<js["groupid"]<<"]:"<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()
                        <<" said: "<<js["msg"].get<string>()<<endl;
                }
            }
        }
        /*
        //登录成功，启动接收线程负责接收数据；
        std::thread readTask(readTaskHandler, clientfd);//实际调用的linux的c的pthread_create
        readTask.detach();//pthread_detach
         */
        g_isLoginSuccess=true;

    }
}

//处理注册的响应逻辑
void doRegResponse(json &responsejs){
    //json responsejs = json::parse(buffer);//string->js 子线程已经完成
    if(0!=responsejs["errno"].get<int>()){//注册失败，返回注册失败错误码
        cerr<<"name is already exist, register error!"<<endl;
    }
    else{//注册成功，返回注册成功及相关信息
        cout<<"name register success, userid is "<<responsejs["id"]
            <<", do not forget it!"<<endl;
    }
}

//接收线程实现
void readTaskHandler(int clientfd){
    for(;;)
    {
        char buffer[1024]={0};
        int len=recv(clientfd,buffer,1024,0);
        if(-1==len || 0==len){
            close(clientfd);
            exit(-1);
        }
        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json js=json::parse(buffer);
        int msgtype=js["msgid"].get<int>();
        if(ONE_CHAT_MSG == msgtype){//打印onechat信息收到的一对一消息
            cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()
                <<" said:"<<js["msg"].get<string>()<<endl;
            continue;
        }
        if(GROUP_CHAT_MSG==msgtype){
            cout<<"群消息["<<js["groupid"]<<"]:"<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()
                <<" said: "<<js["msg"].get<string>()<<endl;
            continue;
        }

        if(LOGIN_MSG_ACK==msgtype)
        {
            cout<<"是登录消息"<<endl;
            doLoginResponse(js);//处理登录响应的业务逻辑
            sem_post(&rwsem);//告知信号量,通知登录结果处理完成
            continue;
        }

        if(REG_MSG_ACK==msgtype)
        {
            cout<<"是注册消息"<<endl;
            doRegResponse(js);
            sem_post(&rwsem);//告知信号量,通知注册结果处理完成
            continue;
        }
    }
}

//客户端命令对应执行函数声明
//"help" command handler
void help(int fd=0,string str="");
//"chat" command handler
void chat(int fd=0,string str="");
//"addfriend" command handler
void addfriend(int fd=0,string str="");
//"creategroup" command handler
void creategroup(int fd=0,string str="");
//"addgroup" command handler
void addgroup(int fd=0,string str="");
//"groupchat" command handler
void groupchat(int fd=0,string str="");
//"loginout" command handler
void loginout(int fd=0,string str="");

//系统支持的客户端命令列表(命令说明)
unordered_map<string,string> commandMap={
        {"help","显示所有支持的命令，格式help"},
        {"chat","一对一聊天，格式chat:friendid:message"},
        {"addfriend","添加好友，格式addfriend:friendid"},
        {"creategroup","创建群组，格式creategroup:groupname:groupdesc"},
        {"addgroup","加入群组，格式addgroup:groupid"},
        {"groupchat","群聊，格式groupchat:groupid:message"},
        {"loginout","注销，格式loginout"}
};

//注册系统支持的客户端命令处理 (登录后客户端的命令和对应方法映射map)
unordered_map<string,function<void(int,string)>> commandHandlerMap={
        {"help",help},
        {"chat",chat},
        {"addfriend",addfriend},
        {"creategroup",creategroup},
        {"addgroup",addgroup},
        {"groupchat",groupchat},
        {"loginout",loginout}
};

//主聊天页面程序 (登录后功能界面)
void mainMenu(int clientfd){
    help();

    char buffer[1024]={0};
    while(isMainMenuRunning){
        //输入“命令：命令内容”
        cin.getline(buffer,1024);
        //备份输入，以供操作
        string commandbuf(buffer);
        //对输入进行命令拆分
        string command;//存储命令
        int idx=commandbuf.find(":");//找到字符串命令尾
        if(-1==idx){//没有找到命令尾':',如 help命令，无需其他参数
            command=commandbuf;
        }
        else{//找到命令尾':'
            command=commandbuf.substr(0,idx);//把命令存在command里
        }
        //依照拆分出的命令通过map表找到命令对应函数
        auto it=commandHandlerMap.find(command);
        if(it == commandHandlerMap.end()){//没找到相关映射，打印无效输入反馈
            cerr<<"invalid input command!"<<endl;
            continue;
        }

        //调用对应命令回调函数，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx+1,commandbuf.size()-idx));
    }
}

//客户端命令对应执行函数实现
//"help" command handler //把帮助key-value表输出告知登录者可执行命令，和命令格式
void help(int fd,string str){//对应输入：无，依托默认值
    cout<<"show command list"<<endl;
    for(auto &p:commandMap){
        cout<<p.first<<":"<<p.second<<endl;
    }
    cout<<endl;
}
//"chat" command handler
void chat(int clientfd,string str){//对应输入：friendid:message
    //解析命令符后的命令内容
    int idx=str.find(":");//js的key尾
    if(-1==idx){//拆分不了，命令错误
        cerr<<"chat command invalid!"<<endl;
        return;
    }

    //js信息组装
    int friendid=atoi(str.substr(0,idx).c_str());//获得朋友id
    string message=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["toid"]=friendid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();cout<<buffer<<endl;
    //发送
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    cout<<len<<endl;
    if(-1==len){
        cerr<<"send chat msg error -> "<<buffer<<endl;
    }
}
//"addfriend" command handler
void addfriend(int clientfd,string str)
{
    //js信息组装
    int friendid=atoi(str.c_str());
    json js;
    js["msgid"]= ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();//序列化

    //发送                                           注：socket发送还是以“xxx\0”即char*形式发送
    int len=send(clientfd,buffer.c_str(), strlen(buffer.c_str())+1,0);
    if(-1==len){//发送失败反馈一下
        cerr<<"send addfriend msg error -> "<<buffer<<endl;
    }
}
//"creategroup" command handler
void creategroup(int clientfd,string str){//对应输入：groupname:groupdesc
    int idx=str.find(":");
    if(-1==idx){
        cerr<<"creategroup command invalid!"<<endl;
        return;
    }

    string groupname=str.substr(0,idx);//groupname
    string groupdesc=str.substr(idx+1,str.size()-idx);//groupdesc

    json js;
    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len){//发送失败反馈一下
        cerr<<"send creategroup msg error -> "<<buffer<<endl;
    }
}
//"addgroup" command handler
void addgroup(int clientfd,string str){//对应输入：groupid
    int groupid= atoi(str.c_str());
    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupid"]=groupid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(), strlen(buffer.c_str())+1,0);
    if(-1==len){
        cerr<<"send addgroup msg error -> "<<buffer<<endl;
    }
}
//"groupchat" command handler
void groupchat(int clientfd,string str){//对应输入：groupid:message
    int idx=str.find(":");
    if(-1==idx){
        cerr<<"groupchat command invalid!"<<endl;
        return;
    }

    int groupid= atoi(str.substr(0,idx).c_str());//groupid
    string message=str.substr(idx+1,str.size()-idx);//message

    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["groupid"]=groupid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len){//发送失败反馈一下
        cerr<<"send groupchat msg error -> "<<buffer<<endl;
    }
}
//"loginout" command handler
void loginout(int clientfd,string str){
    //js信息组装
    json js;
    js["msgid"]= LOGINOUT_MSG;
    js["id"]=g_currentUser.getId();
    string buffer=js.dump();//序列化

    //发送                                           注：socket发送还是以“xxx\0”即char*形式发送
    int len=send(clientfd,buffer.c_str(), strlen(buffer.c_str())+1,0);
    if(-1==len){//发送失败反馈一下
        cerr<<"send loginout msg error -> "<<buffer<<endl;
    }
    else{
        isMainMenuRunning= false;
    }
}

// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}



