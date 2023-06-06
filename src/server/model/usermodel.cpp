//
// Created by lenovo on 2023/5/25.
//
#include "../../../include/server/model/usermodel.hpp"
#include"../../../include/server/db/db.h"
#include<iostream>
using namespace std;

//User表的增加表行的方法
bool UserModel::insert(User &user){
    //1.组装SQL语句
    char sql[1024]={0};
    //输入sql语句到sql中：
    sprintf(sql,"insert into user(name,password,state) values('%s','%s','%s')",
            user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str());

    MySQL mysql;
    if(mysql.connect()){//连接数据库
        if(mysql.update(sql)){//插入sql语句进入数据库
            //获取插入成功的用户数据生成的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));//获得MYSQL类的封装类MySQL的MYSQL对象，再用全局方法获得生成的id,用以设置
            return true;
        }
    }
    return false;
}

//根据用户号码查询用户信息
User UserModel::query(int id){
    //1.组装SQL语句
    char sql[1024]={0};
    //输入sql语句到sql中：
    sprintf(sql,"select * from user where id=%d",id);

    MySQL mysql;
    if(mysql.connect()){//连接数据库
        MYSQL_RES *res=mysql.query(sql); //调创建的MYSQL封装对象mysql查询sql语句
        if(res!= nullptr){//查到
            MYSQL_ROW row=mysql_fetch_row(res);//将查询到的结果拿出一行,返回一个char*数组
            if(row!= nullptr){//找到内容非空，用User对象带值返回
                User user;
                user.setId(atoi(row[0]));//字符串转数字
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);//由于MYSQL在堆区开辟了存储的搜索结果，要释放一下，否则有内存泄露风险
                return user;
            }
        }
    }
    //若返回id为-1的匿名构造默认user对象，即是查询失败
    return User();//User(int id=-1,string name="",string pwd="",string state="offline")
}

//更新用户的状态信息
bool UserModel::updateState(User user){
    //1.组装SQL语句
    char sql[1024]={0};
    //输入sql语句到sql中：
    sprintf(sql,"update user set state = '%s' where id = %d",
            user.getState().c_str(),user.getId());

    MySQL mysql;
    if(mysql.connect()){//连接数据库
        if(mysql.update(sql)){//插入sql语句进入数据库
            return true;
        }
    }
    return false;
}

//重置用户的状态信息
void UserModel::resetState(){
    //组装sql语句
    char sql[1024]="update user set state='offline' where state='online'";

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}
