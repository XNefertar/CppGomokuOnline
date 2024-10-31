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
#define LOGIN_USER(username, password) \
    "select id, score, total_count, win_count from User where username='" + username + "' and password=password('" + password + "');"

inline std::string INSERT_USER(const std::string& name, const std::string& password) {
    return "insert into " + std::string(USER_TABLE_NAME) + " (name, password) values('" + name + "', '" + password + "');";
}

inline std::string DELETE_USER(const std::string& name) {
    return "delete from " + std::string(USER_TABLE_NAME) + " where name = '" + name + "';";
}

inline std::string SELECT_BY_NAME(const std::string& username) {
    return "select id, score, total_count, win_count from " + std::string(USER_TABLE_NAME) + " where username='" + username + "';";
}

inline std::string UPDATE_WIN_USER(int id) {
    return "update " + std::string(USER_TABLE_NAME) + " set score=score+30, total_count=total_count+1, win_count=win_count+1 where id=" + std::to_string(id) + ";";
}

inline std::string UPDATE_LOSE_USER(int id) {
    return "update " + std::string(USER_TABLE_NAME) + " set score=score-10, total_count=total_count+1 where id=" + std::to_string(id) + ";";
}

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
              const std::string &dbname = USER_DB_NAME,
              const int &port = PORT)
    {
        _mysql = MySQL::mysql_create(host, user, passwd, dbname);
        if (_mysql == NULL)
        {
            // Log::LogMessage(ERROR, "UserTable init error: MySQL connection failed");
            return;
        }
        // Log::LogMessage(INFO, "UserTable init success");
    }

    ~UserTable()
    {
        if (_mysql != NULL)
        {
            MySQL::mysql_release(_mysql);
            // Log::LogMessage(INFO, "UserTable destroy");
        }
    }

    // 插入用户
    bool InsertUser(Json::Value &user)
    {
        // std::lock_guard<std::mutex> lock(_mutex);
        if (user["password"].asString().empty() || user["name"].asString().empty())
        {
            // Log::LogMessage(ERROR, "Insert user error: name or password is empty");
            return false;
        }
        std::string name = user["name"].asString();
        std::string password = user["password"].asString();
        std::string sql = INSERT_USER(name, password);
        if (MySQL::MySQL_Execute(_mysql, sql.c_str()) != 0)
        {
            // Log::LogMessage(ERROR, "Insert user error: %s", mysql_error(_mysql));
            return false;
        }
        return true;
    }

    // 登录验证
    bool Login(Json::Value &user)
    {
        std::cout << "Login Test..." << std::endl;
        if (user["password"].isNull() || user["username"].isNull())
        {
            // Log::LogMessage(ERROR, "Login error: username or password is empty");
            return false;
        }
        std::cout << "Login Test 1..." << std::endl;
        std::string username = user["username"].asString();
        std::string password = user["password"].asString();

        std::cout << "username: " << username << std::endl;
        std::cout << "password: " << password << std::endl;
        std::string sql = LOGIN_USER(username, password);
        // 作用域内定义锁
        // 限制互斥锁的控制范围
        // 作用域结束，锁自动释放
        MYSQL_RES *res = nullptr;
        {
            // Problem Here
            // TODO: Fix this problem
            std::cout << "Login Test 2..." << std::endl;
            std::lock_guard<std::mutex> lock(_mutex);
            if (!MySQL::MySQL_Execute(_mysql, sql.c_str()))
            {
                // Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
                return false;
            }
            res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                // Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
                return false;
            }
        }
        std::cout << "Login Test 3..." << std::endl;
        // 获取结果集
        int RowNum = mysql_num_rows(res);
        if (RowNum != 1)
        {
            // Log::LogMessage(ERROR, "Login error: %s", mysql_error(_mysql));
            return false;
        }
        
        std::cout << "Login Test 4..." << std::endl;
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
    bool SelectByUID(uint64_t id, Json::Value &user)
    {
        std::string sql = SELECT_BY_NAME(std::to_string(id));
        MYSQL_RES *res = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!MySQL::MySQL_Execute(_mysql, sql.c_str()))
            {
                // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
            res = mysql_store_result(_mysql);
            if(res == NULL)
            {
                // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
        }
        MYSQL_ROW row = mysql_fetch_row(res);
        if(row == NULL)
        {
            // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
            return false;
        }
        user["id"] = (Json::UInt64)id;
        user["username"] = row[0];
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }

    // 通过用户名获取用户信息
    bool SelectByName(const std::string UserName, Json::Value &user)
    {
        std::string sql = SELECT_BY_NAME(UserName);
        MYSQL_RES *res = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!MySQL::MySQL_Execute(_mysql, sql.c_str()))
            {
                // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
            res = mysql_store_result(_mysql);
            if(res == NULL)
            {
                // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
                return false;
            }
        }
        int RowNum = mysql_num_rows(res);
        if(RowNum != 1)
        {
            // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
            return false;
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        // if(row == NULL)
        // {
        //     // Log::LogMessage(ERROR, "Select user error: %s", mysql_error(_mysql));
        //     return false;
        // }
        user["id"] = (Json::UInt64)std::stol(row[0]);
        user["username"] = UserName;
        user["score"] = (Json::UInt64)std::stol(row[1]);
        user["total_count"] = std::stoi(row[2]);
        user["win_count"] = std::stoi(row[3]);
        mysql_free_result(res);
        return true;
    }

    // 胜利时更新用户信息
    // 天梯分数更新
    // 战斗和胜利次数更新
    bool UpdateWinUser(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        std::string sql = UPDATE_WIN_USER(id);
        if(!MySQL::MySQL_Execute(_mysql, sql.c_str()))
        {
            // Log::LogMessage(ERROR, "Update user error: %s", mysql_error(_mysql));
            return false;
        }
        return true;
    }

    bool UpdateLoseUser(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        
        std::string sql = UPDATE_LOSE_USER(id);
        if(!MySQL::MySQL_Execute(_mysql, sql.c_str()))
        {
            // Log::LogMessage(ERROR, "Update user error: %s", mysql_error(_mysql));
            return false;
        }
        return true;
    }
};

#endif // _USER_TABLE_HPP_