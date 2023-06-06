//
// Created by lenovo on 2023/5/27.
//

#include "../../../include/server/model/offlinemessagemodel.hpp"
#include "../../../include/server/db/db.h"

//存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg){
    //1.组装SQL语句
    char sql[1024]={0};
    //输入sql语句到sql中：
    sprintf(sql,"insert into offlinemessage values(%d,'%s')",userid,msg.c_str());

    MySQL mysql;
    if(mysql.connect()){//连接数据库
        mysql.update(sql);
    }
}

//删除用户的离线消息
void OfflineMsgModel::remove(int userid){
    //1.组装SQL语句
    char sql[1024]={0};
    //输入sql语句到sql中：
    sprintf(sql,"delete from offlinemessage where userid=%d",userid);

    MySQL mysql;
    if(mysql.connect()){//连接数据库
        mysql.update(sql);
    }
}

//查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid){
    //1.组装SQL语句
    char sql[1024]={0};
    //输入sql语句到sql中：
    sprintf(sql,"select message from offlinemessage where userid=%d",userid);

    vector<string> vec;//返回id的离线消息数组
    MySQL mysql;
    if(mysql.connect()){//连接数据库
        MYSQL_RES *res=mysql.query(sql); //调创建MYSQL的封装对象mysql查询sql语句
        if(res!= nullptr){//查到
            //把userid用户的所有离线消息放入vec中返回
            MYSQL_ROW row;
            while((row= mysql_fetch_row(res))!= nullptr){//一行一行读完
                vec.push_back(row[0]);
            }
            mysql_free_result(res);//res来自于堆区，析构一下
            return vec;
        }
    }
    //若返回id为-1的匿名构造默认user对象，即是查询失败
    return vec;
}
