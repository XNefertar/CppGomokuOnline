#ifndef _GAME_SERVER_HPP_
#define _GAME_SERVER_HPP_

#include "MySQL.hpp"
#include "Logger.hpp"
#include "FileRead.hpp"
#include "JsonCpp.hpp"
#include "OnlineManage.hpp"
#include "UserTable.hpp"
#include "RoomManage.hpp"
#include "Matcher.hpp"
#include "StringSplit.hpp"
#include "SessionManage.hpp"

#define WWWROOT         "./wwwroot/"

class GameServer
{
private:
    std::string     _WebRoot;
    wsserver_t      _Server;
    UserTable       _UserTable;
    OnlineManage    _OnlineManage;
    RoomManage      _RoomManage;
    Matcher         _Matcher;
    SessionManage   _SessionManage;

private:
    void HandlerFile(wsserver_t::connection_ptr con)
    {
        websocketpp::http::parser::request HttpRequst = con->get_request();
        std::string URI = HttpRequst.get_uri();
        std::string FilePath = _WebRoot + URI;

        if (FilePath.back() == '/')
        {
            FilePath += "index.html";
        }

        Json::Value resp;
        std::string body;
        // 读取失败
        // 返回404 Not Found
        // 返回格式为html
        if (!FileRead::Read(FilePath, body))
        {
            body += "<html><head><meta charset='UTF-8'/></head><body><h1>404 Not Found</h1></body></html>";
            con->set_status(websocketpp::http::status_code::not_found);
            con->set_body(body);
            return;
        }
        con->set_body(body);
        con->set_status(websocketpp::http::status_code::ok);
    }

    void HttpResp(wsserver_t::connection_ptr &con, bool result,\
     websocketpp::http::status_code::value code, const std::string &reason)
    {
        Json::Value resp;
        resp["Result"] = result ? "true" : "false";
        resp["Reason"] = reason;
        std::string str;
        JsonCpp::serialize(resp, str);
        con->set_body(str);
        con->set_status(code);
        con->append_header("Content-Type", "application/json");
        return;
    }

    // 注册
    void Reg(wsserver_t::connection_ptr &con)
    {
        websocketpp::http::parser::request HttpRequest = con->get_request();
        std::string ReqBody = HttpRequest.get_body();
        Json::Value RegInfo;
        if (!JsonCpp::deserialize(ReqBody, RegInfo))
        {
            ERR_LOG("Deserialize error");
            return HttpResp(con, false, websocketpp::http::status_code::bad_request, "Bad Request");
        }
        INF_LOG("Deserialize Success");
        std::cout << "ReqBody: " << ReqBody << std::endl;

        if (RegInfo["username"].isNull() || RegInfo["password"].isNull())
        {
            ERR_LOG("Username or password is empty");
            return HttpResp(con, false, websocketpp::http::status_code::bad_request, "请输入用户名或密码");
        }
        INF_LOG("Username or password is not empty");

        if (!_UserTable.InsertUser(RegInfo))
        {
            ERR_LOG("Insert user error");
            return HttpResp(con, false, websocketpp::http::status_code::internal_server_error, "用户名已被占用");
        }
        INF_LOG("Insert User Success");
        return HttpResp(con, true, websocketpp::http::status_code::ok, "注册成功");
    }

    void Login(wsserver_t::connection_ptr &con)
    {
        websocketpp::http::parser::request HttpRequst = con->get_request();
        // 获取正文
        std::string content = HttpRequst.get_body();
        // DBG_LOG("content: %s", content.c_str());
        Json::Value LoginInfo;
        if (!JsonCpp::deserialize(content, LoginInfo))
        {
            // Log::LogMessage(ERROR, "Deserialize error");
            ERR_LOG("Deserialize error");
            return HttpResp(con, false, websocketpp::http::status_code::bad_request, "Bad Request");
        }
        // DBG_LOG("Deserialize Success");

        // 校验正文完整性
        if (LoginInfo["username"].isNull() || LoginInfo["password"].isNull())
        {
            // Log::LogMessage(ERROR, "Username or password is empty");
            ERR_LOG("Username or password is empty");
            return HttpResp(con, false, websocketpp::http::status_code::bad_request, "请输入用户名或密码");
        }
        // DBG_LOG("Username or password is not empty");

        if (!_UserTable.Login(LoginInfo))
        {
            // Log::LogMessage(ERROR, "Login error");
            ERR_LOG("Login error");
            return HttpResp(con, false, websocketpp::http::status_code::internal_server_error, "用户名或密码错误");
        }
        // DBG_LOG("Login Success");
        // 如果登录成功，创建会话
        SessionPtr ssp = _SessionManage.CreateSession(LoginInfo["id"].asUInt64(), SESSION_STATE::LOGIN);
        if (ssp.get() == nullptr)
        {
            // Log::LogMessage(ERROR, "Create session error");
            ERR_LOG("Create session error");
            return HttpResp(con, false, websocketpp::http::status_code::internal_server_error, "创建会话失败");
        }
        // DBG_LOG("Create Session Success");

        // 设置会话过期时间
        _SessionManage.SetSessionExpireTime(ssp->GetSessionID(), SESSION_TIMEOUT);

        // DBG_LOG("Set Session Expire Time Success");
        // 设置响应头部
        std::string cookie_ssid = "SSID=" + std::to_string(ssp->GetSessionID());
        con->append_header("Set-Cookie", cookie_ssid);
        // DBG_LOG("con->append_header Success : %s", con->get_request().get_header("Cookie").c_str());
        INF_LOG("Login Success");
        return HttpResp(con, true, websocketpp::http::status_code::ok, "登录成功");
    }

    bool GetCookieVal(const std::string &cookie, const std::string &key, std::string &val)
    {
        // Cookie 格式：key1=val1; key2=val2; key3=val3
        // 以 ; 分割
        std::vector<std::string> cookies;
        StringSplit::Split(cookie, ";", cookies);

        // 以 ; 作为间隔符，分割后的字符串格式为 key=val
        for (auto &it : cookies)
        {
            std::vector<std::string> kv;
            StringSplit::Split(it, "=", kv);
            if (kv.size() != 2)
            {
                continue;
            }
            if (kv[0] == key)
            {
                val = kv[1];
                return true;
            }
        }
        ERR_LOG("Get Cookie Value Error");
        return false;
    }

    // 用户信息获取功能
    // 通过Cookie中的SessionID获取用户信息
    void UserGetInfo(wsserver_t::connection_ptr &con)
    {
        websocketpp::http::parser::request HttpRequest = con->get_request();
        std::string cookie = HttpRequest.get_header("Cookie");

        DBG_LOG("Cookie: %s", cookie.c_str());

        std::string sessionID;
        if (!GetCookieVal(cookie, "SSID", sessionID))
        {
            // Log::LogMessage(ERROR, "Get SessionID error");
            ERR_LOG("Get SessionID error");
            return HttpResp(con, false, websocketpp::http::status_code::bad_request, "Error To Find Cookie");
        }
        DBG_LOG("SessionID: %s", sessionID.c_str());

        // std::stoull: string to unsigned long long
        // 获取会话
        uint64_t sid = std::stoull(sessionID);
        SessionPtr ssp = _SessionManage.GetSessionBySID(sid);
        if (ssp.get() == nullptr)
        {
            // Log::LogMessage(ERROR, "Get session error");
            ERR_LOG("Get session error");
            return HttpResp(con, false, websocketpp::http::status_code::bad_request, "Bad Request");
        }

        // 获取用户信息
        Json::Value user;
        if (!_UserTable.SelectByUID(ssp->GetUID(), user))
        {
            // Log::LogMessage(ERROR, "Select user error");
            ERR_LOG("Select user error");
            return HttpResp(con, false, websocketpp::http::status_code::internal_server_error, "Internal Server Error");
        }

        // 返回用户信息
        std::string str;
        JsonCpp::serialize(user, str);
        con->set_body(str);
        con->set_status(websocketpp::http::status_code::ok);
        con->append_header("Content-Type", "application/json");

        // 设置会话过期时间
        _SessionManage.SetSessionExpireTime(ssp->GetSessionID(), SESSION_TIMEOUT);
        INF_LOG("Get User Info Success");
        return;
    }

    // Handler句柄，处理HTTP请求
    // 通过Method判断请求类型
    void HttpCallback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr con = _Server.get_con_from_hdl(hdl);
        websocketpp::http::parser::request HttpRequest = con->get_request();
        std::string Method = HttpRequest.get_method();
        std::string URI = HttpRequest.get_uri();
        // INF_LOG("Receive Request: %s %s", Method.c_str(), URI.c_str());
        if (Method == "POST" && URI == "/reg")
        {
            // INF_LOG("Calling Reg function");
            Reg(con);
        }
        else if (Method == "POST" && URI == "/login")
        {
            // INF_LOG("Calling Login function");
            Login(con);
        }
        else if (Method == "GET" && URI == "/info")
        {
            // INF_LOG("Calling UserGetInfo function");
            UserGetInfo(con);
        }
        else
        {
            // INF_LOG("Calling HandlerFile function");
            HandlerFile(con);
        }
    }

    void WebsocketResp(wsserver_t::connection_ptr &con, Json::Value &resp)
    {
        std::string str;
        JsonCpp::serialize(resp, str);
        con->send(str);
    }

    SessionPtr GetSessionByCookie(wsserver_t::connection_ptr con)
    {
        Json::Value ErrorResp;
        const std::string &cookie = con->get_request().get_header("Cookie");
        if (cookie.empty())
        {
            ErrorResp["OpType"] = "HallReady";
            ErrorResp["Result"] = "false";
            ErrorResp["Reason"] = "Error To Find Cookie";
            WebsocketResp(con, ErrorResp);
            ERR_LOG("Error To Find Cookie");
            return nullptr;
        }

        std::string sessionID;
        if (!GetCookieVal(cookie, "SSID", sessionID))
        {
            ErrorResp["OpType"] = "HallReady";
            ErrorResp["Result"] = "false";
            ErrorResp["Reason"] = "Error To Find Cookie";
            WebsocketResp(con, ErrorResp);
            ERR_LOG("Error To Find Cookie");
            return nullptr;
        }

        SessionPtr ssp = _SessionManage.GetSessionBySID(std::stoull(sessionID));
        if (ssp.get() == nullptr)
        {
            ErrorResp["OpType"] = "HallReady";
            ErrorResp["Result"] = "false";
            ErrorResp["Reason"] = "Error To Find Session";
            WebsocketResp(con, ErrorResp);
            ERR_LOG("Error To Find Session");
            return nullptr;
        }
        return ssp;
    }

    void WebsocketOpenGameHall(wsserver_t::connection_ptr con)
    {
        Json::Value resp;
        // 登录验证
        SessionPtr ssp = GetSessionByCookie(con);
        if (ssp.get() == nullptr)
        {
            ERR_LOG("Get Session Error");
            return;
        }
        // 判断当前客户端是否重复登录
        if (_OnlineManage.IsInGameHall(ssp->GetUID()) || _OnlineManage.IsInGameRoom(ssp->GetUID()))
        {
            resp["OpType"] = "HallReady";
            resp["Result"] = "false";
            resp["Reason"] = "Already In GameHall";
            WebsocketResp(con, resp);
            ERR_LOG("Already In GameHall");
            return;
        }

        // 将当前客户端以及连接加入游戏大厅
        _OnlineManage.EnterGameHall(ssp->GetUID(), con);

        // 返回成功信息
        resp["OpType"] = "HallReady";
        resp["Result"] = "true";
        resp["Reason"] = "Enter GameHall Success";
        WebsocketResp(con, resp);
        INF_LOG("Enter GameHall Success");
        // 将Session设置为永久存在
        _SessionManage.SetSessionExpireTime(ssp->GetSessionID(), SESSION_FOREVER);
    }

    void WebsocketOpenGameRoom(wsserver_t::connection_ptr con)
    {
        Json::Value resp;
        // 登录验证
        SessionPtr ssp = GetSessionByCookie(con);
        if (ssp.get() == nullptr)
        {
            ERR_LOG("Get Session Error");
            return;
        }

        // 判断当前客户端是否重复登录
        if (_OnlineManage.IsInGameHall(ssp->GetUID()) || _OnlineManage.IsInGameRoom(ssp->GetUID()))
        {
            resp["OpType"] = "RoomReady";
            resp["Result"] = "false";
            resp["Reason"] = "Already In GameRoom";
            WebsocketResp(con, resp);
            ERR_LOG("Already In GameRoom");
            return;
        }

        // 判断当前用户是否已经创建好房间
        RoomPtr room = _RoomManage.GetRoomByUID(ssp->GetUID());
        if (room.get() == nullptr)
        {
            resp["OpType"] = "RoomReady";
            resp["Result"] = "false";
            resp["Reason"] = "Not In GameRoom";
            WebsocketResp(con, resp);
            ERR_LOG("Not In GameRoom");
            return;
        }

        // 将当前客户端以及连接加入游戏房间
        _OnlineManage.EnterGameRoom(ssp->GetUID(), con);

        // 返回成功信息
        resp["OpType"] = "RoomReady";
        resp["Result"] = "true";
        resp["Reason"] = "Enter GameRoom Success";
        WebsocketResp(con, resp);

        // 将Session设置为永久存在
        _SessionManage.SetSessionExpireTime(ssp->GetSessionID(), SESSION_FOREVER);
        INF_LOG("Enter GameRoom Success");
        return;
    }

    void WebsocketOpenCallback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr con = _Server.get_con_from_hdl(hdl);
        websocketpp::http::parser::request HttpRequest = con->get_request();
        std::string URI = HttpRequest.get_uri();
        if (URI == "/gamehall")
        {
            WebsocketOpenGameHall(con);
            return;
        }
        else if (URI == "/gameroom")
        {
            WebsocketOpenGameRoom(con);
            return;
        }
    }

    void WebsocketCloseGameHall(wsserver_t::connection_ptr con)
    {
        Json::Value resp;
        // 登录验证
        SessionPtr ssp = GetSessionByCookie(con);
        if (ssp.get() == nullptr)
        {
            ERR_LOG("Get Session Error");
            return;
        }

        // 将当前客户端以及连接断开游戏大厅
        _OnlineManage.ExitGameHall(ssp->GetUID());

        // 将Session设置
        _SessionManage.SetSessionExpireTime(ssp->GetSessionID(), SESSION_TIMEOUT);
    }

    void WebsocketCloseGameRoom(wsserver_t::connection_ptr con)
    {
        SessionPtr ssp = GetSessionByCookie(con);
        if (ssp.get() == nullptr)
        {
            ERR_LOG("Get Session Error");
            return;
        }

        // 将当前客户端以及连接断开游戏房间
        _OnlineManage.ExitGameRoom(ssp->GetUID());

        // 将Session设置
        _SessionManage.SetSessionExpireTime(ssp->GetSessionID(), SESSION_TIMEOUT);
    }

    void WebsocketCloseCallback(websocketpp::connection_hdl hdl)
    {
        wsserver_t::connection_ptr con = _Server.get_con_from_hdl(hdl);
        std::string URI = con->get_request().get_uri();
        if (URI == "/gamehall")
        {
            WebsocketCloseGameHall(con);
        }
        else if (URI == "/gameroom")
        {
            WebsocketCloseGameRoom(con);
        }
    }

    void WebsocketMessageGameHall(wsserver_t::connection_ptr con, wsserver_t::message_ptr msg)
    {
        Json::Value resp;
        std::string content;
        SessionPtr ssp = GetSessionByCookie(con);
        if (ssp.get() == nullptr)
        {
            ERR_LOG("Get Session Error");
            return;
        }

        // 获取请求信息
        std::string message = msg->get_payload();
        Json::Value req;
        if (!JsonCpp::deserialize(message, req))
        {
            resp["OpType"] = "HallMessage";
            resp["Result"] = "false";
            resp["Reason"] = "Deserialize Error";
            WebsocketResp(con, resp);
            ERR_LOG("Deserialize Error");
            return;
        }

        if (req["OpType"].isString() && req["OpType"].asString() == "MatchStart")
        {
            _Matcher.AddMatch(ssp->GetUID());
            resp["OpType"] = "MatchStart";
            resp["Result"] = "true";
            return WebsocketResp(con, resp);
        }
        else if (req["OpType"].isString() && req["OpType"].asString() == "MatchCancel")
        {
            _Matcher.DelMatch(ssp->GetUID());
            resp["OpType"] = "MatchCancel";
            resp["Result"] = "true";
            return WebsocketResp(con, resp);            
        }
        resp["OpType"] = "UNKNOWN";
        resp["Reason"] = "Unknown Request";
        resp["Result"] = "true";
        ERR_LOG("Unknown Request");
        return WebsocketResp(con, resp);
    }

    void WebsocketMessageGameRoom(wsserver_t::connection_ptr con, wsserver_t::message_ptr msg)
    {
        Json::Value resp;
        SessionPtr ssp = GetSessionByCookie(con);
        if (ssp.get() == nullptr)
        {
            ERR_LOG("Get Session Error");
            return;
        }

        // 获取客户端房间信息
        RoomPtr room = _RoomManage.GetRoomByUID(ssp->GetUID());
        if (room.get() == nullptr)
        {
            resp["OpType"] = "Unknown";
            resp["Result"] = "false";
            resp["Reason"] = "Not In GameRoom";
            WebsocketResp(con, resp);
            ERR_LOG("Not In GameRoom");
            // Log::LogMessage(ERROR, "Not In GameRoom");
            return;
        }

        // 对消息进行反序列化处理
        Json::Value req;
        std::string body = msg->get_payload();
        if (!JsonCpp::deserialize(body, req))
        {
            resp["OpType"] = "Unknown";
            resp["Result"] = "false";
            resp["Reason"] = "Deserialize Error";
            ERR_LOG("Deserialize Error");
            return WebsocketResp(con, resp);
        }

        DBG_LOG("Receive Request: %s", body.c_str());

        return room->HandlerRequest(req);
    }

    void WebsocketMessageCallback(websocketpp::connection_hdl hdl, wsserver_t::message_ptr msg)
    {
        wsserver_t::connection_ptr con = _Server.get_con_from_hdl(hdl);
        std::string URI = con->get_request().get_uri();
        if (URI == "/gamehall")
        {
            WebsocketMessageGameHall(con, msg);
        }
        else if (URI == "/gameroom")
        {
            WebsocketMessageGameRoom(con, msg);
        }
    }

public:
    GameServer(const std::string &host,
               const std::string &user,
               const std::string &passwd,
               const std::string &dbname,
               uint16_t port = 3306,
               const std::string &webroot = WWWROOT)
        : _WebRoot(webroot),
          _UserTable(host, user, passwd, dbname, port),
          _RoomManage(&_UserTable, &_OnlineManage),
          _SessionManage(&_Server),
          _Matcher(&_RoomManage, &_UserTable, &_OnlineManage)
    {
        _Server.clear_access_channels(websocketpp::log::alevel::all);
        _Server.clear_error_channels(websocketpp::log::elevel::all);
        _Server.init_asio();
        _Server.set_http_handler(std::bind(&GameServer::HttpCallback, this, std::placeholders::_1));
        _Server.set_open_handler(std::bind(&GameServer::WebsocketOpenCallback, this, std::placeholders::_1));
        _Server.set_close_handler(std::bind(&GameServer::WebsocketCloseCallback, this, std::placeholders::_1));
        _Server.set_message_handler(std::bind(&GameServer::WebsocketMessageCallback, this, std::placeholders::_1, std::placeholders::_2));
    }

    void Run(uint16_t port)
    {
        _Server.listen(port);
        _Server.start_accept();
        _Server.run();
    }
};

#endif // _GAME_SERVER_HPP_