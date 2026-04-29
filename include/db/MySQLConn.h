#pragma once
#include <mysql/mysql.h>
#include <string>
#include <iostream>

class MySQLConn {
public:
    // 初始化数据库连接
    MySQLConn();
    // 释放数据库连接资源
    ~MySQLConn();

    // 禁用拷贝，防止指针被重复释放导致崩溃
    MySQLConn(const MySQLConn&) = delete;
    MySQLConn& operator=(const MySQLConn&) = delete;

    // 连接数据库
    bool connect(const std::string& user, const std::string& passwd, 
                 const std::string& dbName, const std::string& ip, 
                 unsigned short port = 3306);
                 
    // 执行更新操作 (Insert, Update, Delete)
    bool update(const std::string& sql);
    
    // 执行查询操作 (Select)
    bool query(const std::string& sql);
    
    // 遍历查询得到的结果集
    bool next();
    
    // 获取当前记录中某个字段的值 (索引从 0 开始)
    std::string value(int index);

private:
    void freeResult(); // 内部用来释放查询结果集的内存

    MYSQL* conn_;      // MySQL 连接的绝对核心句柄
    MYSQL_RES* result_;// 存放查询返回的结果集
    MYSQL_ROW row_;    // 存放当前遍历到的一行数据
};