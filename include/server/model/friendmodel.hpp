//
// Created by lenovo on 2023/5/27.
//

#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include"user.hpp"
#include<vector>

//维护friend表的操作业务类
class FriendModel{
public:
    //添加好友关系
    void insert(int userid, int friendid);

    //返回用户好友列表（现实中一般存在客户端，这里写成服务端，现实中压力大）
    vector<User> query(int userid);//这里要返回id，还有其他用户信息，所以要联合查询

};

#endif //DEMO_FRIENDMODEL_HPP
