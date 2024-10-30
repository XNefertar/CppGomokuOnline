#ifndef _USER_TABLE_HPP_
#define _USER_TABLE_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <jsoncpp/json/json.h>
#include "MySQL.hpp"
#include "Log.hpp"

#define USER_TABLE_MAX_NUM 1024
#define USER_TABLE_MAX_SIZE 1024
#define USER_TABLE_NAME "User"
#define USER_DB_NAME "Online_Goband"
#define USER_TABLE_HOST "192.144.236.38"
#define USER_TABLE_PASSWORD "XMysql12345"

// SQL 语句宏定义
#define LOGIN_USER        "select id, score, total_count, win_count from User where username='%s' and password=password('%s');"
#define INSERT_USER       "insert into %s (name, password) values('%s', '%s');", USER_TABLE_NAME, name.c_str(), password.c_str()
#define DELETE_USER       "delete from %s where name = '%s';", USER_TABLE_NAME, name.c_str()
#define SELECT_BY_NAME    "select id, score, total_count, win_count from %s where username='%s';", USER_TABLE_NAME
#define UPDATE_WIN_USER   "update %s set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=%d;", USER_TABLE_NAME
#define UPDATE_LOSE_USER  "update %s set score=score-10, total_count=total_count+1 where id=%d;", USER_TABLE_NAME

using namespace LOG_MSG;
class UserTable
{
private:
    MYSQL *_mysql;
    std::mutex _mutex;

public:
    UserTable(const std::string &host = USER_TABLE_HOST,
              const std::string &user = USER_TABLE_NAME,
              const std::string &passwd = USER_TABLE_PASSWORD,
              const std::string &dbname = USER_DB_NAME)
    {
        Log::LogInit("log.txt", NORMAL);
        _mysql = MySQL::mysql_create(host, user, passwd, dbname);
        if (_mysql == NULL)
        {
            Log::LogMessage(ERROR, "UserTable init error");
            return;
        }
        Log::LogMessage(INFO, "UserTable init success");
    }

    ~UserTable()
    {
        MySQL::mysql_close(_mysql);
        Log::LogMessage(INFO, "UserTable destroy");
    }

    // 插入用户
    bool InsertUser(Json::Value &user)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (user["password"].asString().empty() || user["name"].asString().empty())
        {
            Log::LogMessage(ERROR, "Insert user error: name or password is empty");
            return false;
        }
        std::string name = user["name"].asString();
        std::string password = user["password"].asString();
        char sql[USER_TABLE_MAX_SIZE]{};
        sprintf(sql, INSERT_USER);
        if (MySQL::MySQL_Execute(_mysql, sql) != 0)
        {
            Log::LogMessage(ERROR, "Insert user error: %s", mysql_error(_mysql));
            return false;
        }
        return true;
    }

    // 登录验证
    bool Login(Json::Value &user)
    {
        if (user["password"].isNull() || user["username"].isNull())
        {
            Log::LogMessage(ERROR, "Login error: username or password is empty");
            return false;
        }
        std::string username = user["username"].asString();
        std::string password = user["password"].asString();
        char sql[USER_TABLE_MAX_SIZE]{};
        sprintf(sql, LOGIN_USER, username.c_str(), password.c_str());
        // 作用域内定义锁
        // 限制互斥锁的控制范围
        // 作用域结束，锁自动释放
        MYSQL_RES *res = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!MySQL::MySQL_Execute(_mysql, sql))
            {
                Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
                return false;
            }
            res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
                return false;
            }
        }
        
        // 获取结果集
        // 可能存在问题
        // TODO
        //
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row == NULL)
        {
            Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
            return false;
        }
        user["id"] = atoi(row[0]);
        user["username"] = username;
        user["score"] = atoi(row[1]);
        user["total_count"] = atoi(row[2]);
        user["win_count"] = atoi(row[3]);
        mysql_free_result(res);
        return true;
    }


    // 通过用户名获取用户信息
    bool SelectByName(const std::string UserName, Json::Value &user)
    {
        char sql[USER_TABLE_MAX_SIZE]{};
        sprintf(sql, SELECT_BY_NAME, UserName.c_str());
        MYSQL_RES *res = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!MySQL::MySQL_Execute(_mysql, sql))
            {
                Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
            res = mysql_store_result(_mysql);
            if(res == NULL)
            {
                Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        if(row == NULL)
        {
            Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
            return false;
        }
        user["id"] = atoi(row[0]);
        user["username"] = UserName;
        user["score"] = atoi(row[1]);
        user["total_count"] = atoi(row[2]);
        user["win_count"] = atoi(row[3]);
        mysql_free_result(res);
        return true;
    }

    // 胜利时更新用户信息
    // 天梯分数更新
    // 战斗和胜利次数更新
    bool UpdateWinUser(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        char sql[USER_TABLE_MAX_SIZE]{};
        sprintf(sql, UPDATE_WIN_USER, id);
        if(!MySQL::MySQL_Execute(_mysql, sql))
        {
            Log::LogMessage(ERROR, "Update user error: %s", mysql_error(_mysql));
            return false;
        }
        return true;
    }

    bool UpdateLoseUser(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        char sql[USER_TABLE_MAX_SIZE]{};
        sprintf(sql, UPDATE_LOSE_USER, id);
        if(!MySQL::MySQL_Execute(_mysql, sql))
        {
            Log::LogMessage(ERROR, "Update user error: %s", mysql_error(_mysql));
            return false;
        }
        return true;
    }
};

#endif // _USER_TABLE_HPP_