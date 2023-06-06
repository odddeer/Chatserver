/*
muduo网络库给用户提供了两个主要的类
  TcpServer:用于编写服务器程序的
  TcpClient:用于编写客户端程序的

  epoll+线程池
  好处：能够把网络I/O的代码和业务代码区分开
           特点：只用关注：   用户的连接和断开  用户的可读写事件 来开发业务

*/
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<string>
#include<functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;//占位符命名空间

/*基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确Tcpserver构造函数需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
5.设置合适的服务端线程数量，muduo库会自己分配I/O线程和worker线程
*/
class ChatServer{
public:
    ChatServer(EventLoop* loop, //事件循环
               const InetAddress& listenAddr,//ip+port
               const string& nameArg)// 服务器的名字
               :_server(loop,listenAddr,nameArg),_loop(loop){
                    //给服务器注册用户连接的创建和断开回调  c++11绑定器新特性
                    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
                    //给服务器注册用户读写事件回调
                    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
                    //设置服务器端的线程数量  //1个I/O线程 3个worker线程 对应cpu数
                    _server.setThreadNum(4);
               }
    //开启事件循环
    void start(){
        _server.start();
    }
private:
    //专门处理用户的连接创建和断开事件函数 epoll listenfd accept
    void onConnection(const TcpConnectionPtr &conn){ 
        if(conn->connected()){//有连接成功的
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<" state:online"<<endl;
        }
        else{//连接关闭
            cout<<conn->peerAddress().toIpPort()<<"->"<<conn->localAddress().toIpPort()<<" state:offline"<<endl;
            conn->shutdown();// 即close(fd)
            //_loop->quit();//退出epoll,服务器结束工作
        }
    }
    //专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn,//连接
                Buffer *buffer,//缓冲区的封装类
                Timestamp time) //接收到数据的时间信息
    {
        string buf=buffer->retrieveAllAsString();
        cout<<"recv data:"<<buf<<" time:"<<time.toString()<<endl;
        conn->send(buf);//接收到啥发啥回去
    }
    TcpServer _server;// #1 定义一个server对象
    EventLoop *_loop;//  #2 epoll
};

int main(){
    EventLoop loop; // 类似epoll创建
    InetAddress addr("127.0.0.1",6000);//定义socket结构体的ip和port
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listen epoll_ctl添加到epoll上
    loop.loop(); // epoll_wait以阻塞的方式等待新用户连接，已连接用户的读写事件等，然后回调onConnection和onMessage方法
    return 0;
}