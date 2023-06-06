//
// Created by lenovo on 2023/5/28.
//

#include "../../../include/server/model/groupmodel.hpp"
#include "../../../include/server/db/db.h"

//创建群组  只需要插入组名，组描述，组号自动+1 加在allgroup表中
bool GroupModel::createGroup(Group &group){
    char sql[1024]={0};
    sprintf(sql,"insert into allgroup(groupname,groupdesc) values('%s','%s')",
            group.getName().c_str(),group.getDesc().c_str());
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
                          //mysql_insert_id返回数据库最后插入值的ID 值，这里就是组号
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

//加入群组    即每个组都有的一个成员信息表 加在groupuser表中（人，组，人角色）
void GroupModel::addGroup(int userid,int groupid,string role){
    char sql[1024]={0};
    sprintf(sql,"insert into groupuser values(%d, %d, '%s')",
            groupid, userid, role.c_str());
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//查询用户所在群组信息     查找某个用户所属组（groupuser表）的组信息（allgroup表）联合查询
vector<Group> GroupModel::queryGroups(int userid){
    char sql[1024]={0};
    sprintf(sql,"select a.id,a.groupname,a.groupdesc from allgroup a inner join \
    groupuser b on a.id = b.groupid where b.userid=%d", userid);

    vector<Group> groupVec;//存储组信息数组

    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res=mysql.query(sql);//把所有userid所属的群组查出来存放在res中
        if(res!= nullptr){
            MYSQL_ROW row;
            //查出userid所有的群组信息
            while((row= mysql_fetch_row(res))!= nullptr){//解析res结果，把群组信息放group对象中，再放groupVec中
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    //查询该用户所属这些群组的用户信息（user表+groupuser表（角色））来装填Group中的groupuser新user
    for(Group &group:groupVec){
        sprintf(sql,"select a.id,a.name,a.state,b.grouprole from user a \
        inner join groupuser b on b.userid = a.id where b.groupid=%d", group.getId());

        MYSQL_RES *res=mysql.query(sql);
        if(res!= nullptr){
            MYSQL_ROW row;
            while((row= mysql_fetch_row(res))!= nullptr){
                GroupUser user;//user+角色 新user对象
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

//根据指定的groupid查询群组用户id列表，除userid自己，主要用户群聊业务给群组其它成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid,int groupid){
    char sql[1024]={0};                                 //不查自己
    sprintf(sql,"select userid from groupuser where groupid = %d and userid != %d",
            groupid,userid);//在（人，组，角色）groupuser表中找指定组中除自己外的所有用户id

    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res= mysql.query(sql);
        if(res!= nullptr){
            MYSQL_ROW row;
            while((row= mysql_fetch_row(res))!= nullptr){
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}