#ifndef _USER_TABLE_HPP_
#define _USER_TABLE_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <jsoncpp/json/json.h>
#include "MySQL.hpp"
#include "Logger.hpp"

#define USER_TABLE_MAX_NUM  1024
#define USER_TABLE_MAX_SIZE 1024
// #define USER_TABLE_NAME     "user"
// #define USER_DB_NAME        "gobang"
// #define USER_TABLE_HOST     "192.144.236.38"
// #define USER_TABLE_PASSWORD "XMysql12345"

/*
#define user["username"]
#define user["password"]
#define user["id"]
#define user["score"]
#define user["total_count"]
#define user["win_count"]
*/

// SQL 语句宏定义
#define LOGIN_USER       "select id, score, total_count, win_count from user where username='%s' and password=SHA2('%s', 256);"
#define INSERT_USER      "INSERT INTO user (username, password, score, total_count, win_count) VALUES ('%s', SHA2('%s', 256), 1000, 0, 0);"
#define DELETE_USER      "delete from user where username='%s';"
#define SELECT_BY_NAME   "select id, score, total_count, win_count from user where username='%s';"
#define SELECT_BY_ID     "select id, username, score, total_count, win_count from user where id=%d;"
#define UPDATE_WIN_USER  "update user set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=%d;"
#define UPDATE_LOSE_USER "update user set score=score-10, total_count=total_count+1 where id=%d;"

// inline std::string INSERT_USER(const std::string& name, const std::string& password) {
//     return "insert into " + std::string(USE_TABLE) + " (name, password) values('" + name + "', '" + password + "');";
// }
// inline std::string DELETE_USER(const std::string& name) {
//     return "delete from " + std::string(USE_TABLE) + " where name = '" + name + "';";
// }
// inline std::string SELECT_BY_NAME(const std::string& username) {
//     return "select id, score, total_count, win_count from " + std::string(USE_TABLE) + " where username='" + username + "';";
// }
// inline std::string UPDATE_WIN_USER(int id) {
//     return "update " + std::string(USE_TABLE) + " set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=" + std::to_string(id) + ";";
// }
// inline std::string UPDATE_LOSE_USER(int id) {
//     return "update " + std::string(USE_TABLE) + " set score=score-10, total_count=total_count+1 where id=" + std::to_string(id) + ";";
// }

class UserTable
{
private:
    MYSQL *_mysql;
    std::mutex _mutex;

public:
    UserTable(const std::string &host = HOST,
              const std::string &user = USER,
              const std::string &passwd = PASSWD,
              const std::string &dbname = DBNAME,
              const int &port = PORT)
    {
        _mysql = MySQL::mysql_create(host, user, passwd, dbname);
        if (_mysql == NULL)
        {
            ERR_LOG("UserTable init error: MySQL connection failed");
            // Log::LogMessage(ERROR, "UserTable init error: MySQL connection failed");
            return;
        }
        INF_LOG("UserTable init success");
        std::cout << "UserTable init success" << std::endl;
        // Log::LogMessage(INFO, "UserTable init success");
    }

    ~UserTable()
    {
        if (_mysql != NULL)
        {
            MySQL::mysql_release(_mysql);
            INF_LOG("UserTable destroy");
            // Log::LogMessage(INFO, "UserTable destroy");
        }
    }

    // 插入用户
    // 注册
    bool InsertUser(Json::Value &user)
    {
        INF_LOG("Insert User");
        // std::lock_guard<std::mutex> lock(_mutex);
        if (user["password"].asString().empty() || user["username"].asString().empty())
        {
            ERR_LOG("Insert user error: name or password is empty");
            // Log::LogMessage(ERROR, "Insert user error: name or password is empty");
            return false;
        }

        char sql[4096] = {0};
        sprintf(sql, INSERT_USER, user["username"].asCString(), user["password"].asCString());
        INF_LOG("sql = %s", sql);
        if (MySQL::MySQL_Execute(_mysql, sql) == 0)
        {
            ERR_LOG("Insert user error: %s", mysql_error(_mysql));
            // Log::LogMessage(ERROR, "Insert user error: %s", mysql_error(_mysql));
            return false;
        }
        INF_LOG("Insert User Success");
        return true;
    }

    // 登录验证
    bool Login(Json::Value &user)
    {
        if (user["password"].isNull() || user["username"].isNull())
        {
            ERR_LOG("Login error: username or password is empty");
            // Log::LogMessage(ERROR, "Login error: username or password is empty");
            return false;
        }
        char sql[4096] = {0};
        sprintf(sql, LOGIN_USER, user["username"].asCString(), user["password"].asCString());
        printf("sql = %s\n", sql);

        // 作用域内定义锁
        // 限制互斥锁的控制范围
        // 作用域结束，锁自动释放
        MYSQL_RES *res = nullptr;
        {
            // Problem Here
            // TODO: Fix this problem
            std::lock_guard<std::mutex> lock(_mutex);

            auto ret = MySQL::MySQL_Execute(_mysql, sql);
            if (!ret)
            {
                ERR_LOG("Login error: %s", mysql_error(_mysql));
                // Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
                return false;
            }

            res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                ERR_LOG("Login error: %s", mysql_error(_mysql));
                // Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
                return false;
            }
        }
        // 获取结果集
        int RowNum = mysql_num_rows(res);
        if (RowNum != 1)
        {
            ERR_LOG("Login error");
            // Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
            return false;
        }
        
        MYSQL_ROW row = mysql_fetch_row(res);
        // if (row == NULL)
        // {
        //     // Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
        //     return false;
        // }
        user["id"] = (Json::UInt64)std::stol(row[0]);
        user["score"] = (Json::UInt64)std::stoi(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }

    // 通过UID获取用户信息
    bool SelectByUID(int id, Json::Value &user)
    {
        char sql[4096] = {0};
        DBG_LOG("Select User");
        DBG_LOG("id = %d", id);
        sprintf(sql, SELECT_BY_ID, id);

        DBG_LOG("sql = %s", sql);
        DBG_LOG("id = %d", id);
        MYSQL_RES *res = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!MySQL::MySQL_Execute(_mysql, sql))
            {
                ERR_LOG("Select user error");
                // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
            DBG_LOG("Select user success");
            res = mysql_store_result(_mysql);
            if(res == NULL)
            {
                ERR_LOG("Select user error");
                // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
            DBG_LOG("Store user information success");
        }
        int RowNum = mysql_num_rows(res);
        if(RowNum != 1)
        {
            ERR_LOG("Select user error");
            // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
            return false;
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        
        DBG_LOG("Fetch user information success");
        for(int i = 0; i < mysql_num_fields(res); i++)
        {
            DBG_LOG("row[%d] = %s", i, row[i]);
        }

        user["id"] = (Json::UInt64)id;
        user["username"] = row[1];
        user["score"] = (Json::UInt64)std::stol(row[2]);
        user["total_count"] = std::stoi(row[3]);
        user["win_count"] = std::stoi(row[4]);
        mysql_free_result(res);
        return true;
    }

    // 通过用户名获取用户信息
    bool SelectByName(const std::string UserName, Json::Value &user)
    {
        DBG_LOG("Select By Name.");
        DBG_LOG("UserName = %s", UserName.c_str());

        char sql[4096] = {0};
        sprintf(sql, SELECT_BY_NAME, UserName.c_str());

        DBG_LOG("sql = %s", sql);
        MYSQL_RES *res = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!MySQL::MySQL_Execute(_mysql, sql))
            {
                ERR_LOG("Select user error");
                // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
            DBG_LOG("Select user success");
            res = mysql_store_result(_mysql);
            if(res == NULL)
            {
                ERR_LOG("Select user error");
                // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
            DBG_LOG("Store user information success");
        }
        int RowNum = mysql_num_rows(res);
        if(RowNum != 1)
        {
            ERR_LOG("Select user error");
            // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
            return false;
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        // if(row == NULL)
        // {
        //     // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
        //     return false;
        // }
        DBG_LOG("Fetch user information success");
        DBG_LOG("ID : row[0] = %s", row[0]);
        user["id"] = (Json::UInt64)std::stol(row[0]);
        DBG_LOG("user[id] = %d", user["id"].asUInt64());
        user["username"] = UserName;
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        DBG_LOG("Select By Name Success.");
        return true;
    }

    // 胜利时更新用户信息
    // 天梯分数更新
    // 战斗和胜利次数更新
    bool UpdateWinUser(int id)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        char sql[4096] = {0};
        sprintf(sql, UPDATE_WIN_USER, id);
        if(!MySQL::MySQL_Execute(_mysql, sql))
        {
            ERR_LOG("Update user error");
            // Log::LogMessage(ERROR, "Update user error: %s", mysql_error(_mysql));
            return false;
        }
        return true;
    }

    bool UpdateLoseUser(int id)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        char sql[4096] = {0};
        sprintf(sql, UPDATE_LOSE_USER, id);
        if(!MySQL::MySQL_Execute(_mysql, sql))
        {
            ERR_LOG("Update user error");
            // Log::LogMessage(ERROR, "Update user error: %s", mysql_error(_mysql));
            return false;
        }
        return true;
    }
};

#endif // _USER_TABLE_HPP_