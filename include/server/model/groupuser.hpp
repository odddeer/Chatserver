//
// Created by lenovo on 2023/5/28.
//

#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"

//显示组用户信息，user基本信息+组成员角色信息（所以继承了User,并添加了角色信息方法）
class GroupUser:public User{//继承和user表的本体信息类
public:
    void setRole(string role){this->role=role;}
    string getRole(){return this->role;}
private:
    string role;
};

#endif //DEMO_GROUPUSER_HPP
