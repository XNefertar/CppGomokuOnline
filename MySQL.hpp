#ifndef _MYSQL_HPP_
#define _MYSQL_HPP_

// MySQL Done
// 封装MySQL工具类
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <mysql/mysql.h>
#include "Logger.hpp"

#define HOST        "192.144.236.38"
#define USE_TABLE   "user"
#define USER        "xl"
#define PASSWD      "X@mysql12345"
#define DBNAME      "gobang"
#define PORT        3306

/*
#define user["username"]
#define user["password"]
#define user["id"]
#define user["score"]
#define user["total_count"]
#define user["win_count"]
*/

class MySQL
{
public:
    // 创建MySQL句柄
    static MYSQL *mysql_create(const std::string &host = HOST,
                               const std::string &user = USER,
                               const std::string &passwd = PASSWD,
                               const std::string &dbname = DBNAME,
                               const int &port = PORT)
    {
        // 1. 初始化MySQL句柄
        MYSQL *mysql = mysql_init(NULL);
        if (mysql == NULL)
        {
            ERR_LOG("MySQL init error");
            return NULL;
        }

        // 3. 连接到MySQL服务器
        if (mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(), dbname.c_str(), port, NULL, 0) == NULL)
        {
            ERR_LOG("MySQL connection error");
            mysql_close(mysql);
            return NULL;
        }

        // 4. 设置字符集（可选）
        if (mysql_set_character_set(mysql, "utf8") == -1)
        {
            ERR_LOG("MySQL set character set error:");
            mysql_close(mysql);
            return NULL;
        }

        INF_LOG("MySQL connection success");
        std::cout << "MySQL connection success" << std::endl;
        return mysql;
    }

    // 关闭MySQL句柄
    static void mysql_release(MYSQL *mysql)
    {
        if(!mysql)
        {
            return;
        }
        mysql_close(mysql);
        return;
    }

    static void mysql_destroy(MYSQL *mysql)
    {
        if (mysql != NULL)
        {
            mysql_close(mysql);
        }
        return;
    }

    // 执行mysql语句
    static bool MySQL_Execute(MYSQL* mysql, const std::string &sql)
    {

        // 检查数据库连接
        if (mysql_ping(mysql) != 0)
        {
            ERR_LOG("MySQL connection lost: %s", mysql_error(mysql));
            return false;
        }

        // 执行SQL语句
        if (mysql_query(mysql, sql.c_str()) != 0)
        {
            ERR_LOG("Execute SQL error: %s", mysql_error(mysql));
            return false;
        }
        return true;
    }
};

#endif // _MYSQL_HPP_