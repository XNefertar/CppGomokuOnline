#ifndef _MYSQL_HPP_
#define _MYSQL_HPP_

// 封装MySQL工具类
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>
#include <mysql/mysql.h>
#include "log.hpp"

#define HOST "127.0.0.1"
#define USER "root"
#define PASSWD "XMysql12345"
#define DBNAME "test_db"

using namespace LOG_MSG;

class MySQL
{
public:
    // 创建MySQL句柄
    static MYSQL *mysql_create(const std::string &host = HOST,
                               const std::string &user = USER,
                               const std::string &passwd = PASSWD,
                               const std::string &dbname = DBNAME)
    {
        MYSQL *mysql = mysql_init(NULL);
        if (mysql == NULL)
        {
            // std::cerr << "MySQL init error" << std::endl;
            Log::LogMessage(ERROR, "MySQL init error");
            return NULL;
        }

        // 2. 连接mysql服务器
        if (mysql_real_connect(mysql, HOST, USER, PASSWD, DBNAME, 0, NULL, 0) == NULL)
        {
            Log::LogMessage(ERROR, "connect mysql server error: %s\n", mysql_error(mysql));
            return NULL;
        }

        // 3. 设置字符集
        if (mysql_set_character_set(mysql, "utf8") != 0)
        {
            Log::LogMessage(ERROR, "set character error: %s\n", mysql_error(mysql));
            return NULL;
        }

        // 4. 选择数据库
        if (mysql_select_db(mysql, DBNAME) != 0)
        {
            Log::LogMessage(ERROR, "select db error: %s\n", mysql_error(mysql));
            return NULL;
        }
        return mysql;
    }

    // 关闭MySQL句柄
    static void mysql_close(MYSQL *mysql)
    {
        if(!mysql)
        {
            return;
        }
        mysql_close(mysql);
    }

    // 执行mysql语句
    static bool MySQL_Execute(MYSQL* mysql, const std::string &sql)
    {
        int ret = mysql_query(mysql, sql.c_str());
        if (ret != 0)
        {
            Log::LogMessage(ERROR, "execute error: %s\n", mysql_error(mysql));
            return false;
        }
        return true;
    }
};

#endif // _MYSQL_HPP_