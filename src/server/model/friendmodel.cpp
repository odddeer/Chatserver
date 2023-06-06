//
// Created by lenovo on 2023/5/27.
//
#include "../../../include/server/model/friendmodel.hpp"
#include"../../../include/server/db/db.h"

//添加好友关系
void FriendModel::insert(int userid, int friendid){
    char sql[1024]={0};
    sprintf(sql,"insert into friend values(%d,%d)",userid,friendid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);//添加好友关系在friend表
    }
}

//返回用户好友列表（现实中一般存在客户端，这里写成服务端，现实中压力大）
vector<User> FriendModel::query(int userid) {
    char sql[1024]={0};
    //联合查询
    sprintf(sql,"select a.id,a.name,a.state from user a inner join friend b on b.friendid = a.id where b.userid=%d",userid);

    vector<User> vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res= mysql.query(sql);
        if(res!= nullptr){
            MYSQL_ROW row;
            while((row= mysql_fetch_row(res))!= nullptr){
                User user;
                user.setId(atoi(row[0]));//id
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}
