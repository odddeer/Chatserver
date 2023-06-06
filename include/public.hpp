//
// Created by lenovo on 2023/5/24.
//

#ifndef DEMO_PUBLIC_HPP
#define DEMO_PUBLIC_HPP

/*
  server和client的公共文件
*/
enum EnMsgType{
    LOGIN_MSG =1, //登录类消息
    LOGIN_MSG_ACK, //登录回应类消息
    LOGINOUT_MSG, //注销消息
    REG_MSG, //注册类消息
    REG_MSG_ACK,//注册回应类消息
    ONE_CHAT_MSG,//聊天消息
    ADD_FRIEND_MSG,//添加好友消息

    CREATE_GROUP_MSG,//创建群组
    ADD_GROUP_MSG,//加入群组
    GROUP_CHAT_MSG,//群聊天
};

#endif //DEMO_PUBLIC_HPP
