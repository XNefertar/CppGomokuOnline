#ifndef _ROOM_HPP_
#define _ROOM_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <jsoncpp/json/json.h>
#include "Log.hpp"
#include "JsonCpp.hpp"
#include "UserTable.hpp"
#include "OnlineManage.hpp"

#define ROOM_MAX_NUM         5
#define ROOM_MAX_SIZE        1024
#define BOARD_ROW            15
#define BOARD_COL            15
#define CHESS_WHITE_COLOR    0
#define CHESS_BLACK_COLOR    1

using namespace LOG_MSG;
typedef enum
{
    GAME_START = 0,
    GAME_OVER,
}RoomStatus;

// 封装房间管理类
// 房间内包含棋局对战和聊天室
class Room
{
private:
    int          _PlayerNum;         // 玩家数量
    uint16_t     _RoomId;            // 房间ID
    RoomStatus   _Status;            // 房间状态
    uint16_t     _WhiteId;           // 白棋玩家ID
    uint16_t     _BlackId;           // 黑棋玩家ID
    uint16_t     _WinnerId;          // 胜利玩家ID
    std::mutex   _Mutex;             // 互斥锁
    UserTable   *_UserTable;        // 用户列表  TODO 未实现 MySQL
    OnlineManage *_OnlineUser;       // 在线管理类

    std::vector<std::vector<int>> _Board; // 棋盘

public:

    // 判断是否出现五子连珠
    bool five(int row, int col, int offsetRow, int offsetCol, int color)
    {
        // row, col 为当前位置
        // offsetRow, offsetCol 为偏移量
        int count = 1;
        int CurRow = row + offsetRow;
        int CurCol = col + offsetCol;
        // 判断当前位置是否越界，并统计相同颜色的棋子个数
        while(CurRow >= 0 && CurRow < BOARD_ROW && CurCol >= 0 && CurCol < BOARD_COL && _Board[CurRow][CurCol] == color)
        {
            count++;
            CurRow += offsetRow;
            CurCol += offsetCol;
        }
        // 换个方向统计
        CurRow = row - offsetRow;
        CurCol = col - offsetCol;
        while(CurRow >= 0 && CurRow < BOARD_ROW && CurCol >= 0 && CurCol < BOARD_COL && _Board[CurRow][CurCol] == color)
        {
            count++;
            CurRow -= offsetRow;
            CurCol -= offsetCol;
        }
        return count >= 5;
    }

    // 判断是否胜利
    uint16_t CheckWin(int row, int col, int color)
    {
        // 判断各个方向是否出现五子连珠
        if (five(row, col, 0, 1, color) ||
            five(row, col, 1, 0, color) ||
            five(row, col, 1, 1, color) ||
            five(row, col, 1, -1, color))
        {
            return color == CHESS_WHITE_COLOR ? _WhiteId : _BlackId;
        }
        return -1;
    }

    // 初始化棋盘
public:
    Room(uint64_t RoomId, UserTable *UserTable, OnlineManage *OnlineUser)
        : _RoomId(RoomId),
          _Status(GAME_START),
          _WhiteId(CHESS_WHITE_COLOR),
          _BlackId(CHESS_BLACK_COLOR),
          _WinnerId(-1),
          _UserTable(UserTable),
          _OnlineUser(OnlineUser),
          _Board(BOARD_ROW, std::vector<int>(BOARD_COL, -1))
    {
        Log::LogInit("log.txt", NORMAL);
        Log::LogMessage(INFO, "Room %d init success", _RoomId);
    }

    // 重置棋盘
    ~Room()
    {
        Log::LogMessage(INFO, "Room %d destroy", _RoomId);
    }

    // 一系列函数接口用于获取信息
    int         GetPlayerNum() { return _PlayerNum; }
    RoomStatus  GetStatus()    { return _Status; }
    uint16_t    GetRoomId()    { return _RoomId; }
    uint16_t    GetWhiteId()   { return _WhiteId; }
    uint16_t    GetBlackId()   { return _BlackId; }
    uint16_t    GetWinnerId()  { return _WinnerId; }

    // 一系列函数接口用于设置信息
    void AddBlakcPlayer(uint16_t id)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        _BlackId = id;
        _PlayerNum++;
    }
    void AddWhitePlayer(uint16_t id)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        _WhiteId = id;
        _PlayerNum++;
    }

    // 处理下棋动作
    Json::Value HandlerChess(Json::Value &req)
    {
        Json::Value resp = req;
        int row = req["row"].asInt();
        int col = req["col"].asInt();
        uint16_t CurID = req["id"].asInt();
        // 判断玩家是否在线
        if(!_OnlineUser->IsInGameRoom(_WhiteId))
        {
            resp["Result"] = "true";
            resp["Reason"] = "white player is offline";
            resp["Winner"] = (Json::Value::UInt64)_BlackId;
            return resp;
        }
        if(!_OnlineUser->IsInGameRoom(_BlackId))
        {
            resp["Result"] = "true";
            resp["Reason"] = "black player is offline";
            resp["Winner"] = (Json::Value::UInt64)_WhiteId;
            return resp;
        }

        // 判断当前玩家下棋位置的合法性
        if(_Board[row][col] != -1)
        {
            resp["Result"] = "false";
            resp["Reason"] = "this position has been occupied";
            return resp;
        }

        // 将当前位置置为对应的颜色
        int cur_color = CurID == _WhiteId ? CHESS_WHITE_COLOR : CHESS_BLACK_COLOR;
        _Board[row][col] = cur_color;

        // 判断是否有玩家胜利
        uint16_t WinnerID = CheckWin(row, col, cur_color);
        if(WinnerID != -1)
        {
            resp["Result"] = "true";
        }
        resp["Reason"] = "Win";
        resp["Winner"] = (Json::Value::UInt64)WinnerID;
        return resp;
    }

    // 处理聊天动作
    Json::Value HandlerChat(Json::Value &req)
    {
        Json::Value resp = req;
        uint16_t CurID = req["id"].asInt();
        std::string msg = req["msg"].asString();
        // 判断玩家是否在线
        if(!_OnlineUser->IsInGameRoom(_WhiteId))
        {
            resp["Result"] = "false";
            resp["Reason"] = "white player is offline";
            return resp;
        }
        if(!_OnlineUser->IsInGameRoom(_BlackId))
        {
            resp["Result"] = "false";
            resp["Reason"] = "black player is offline";
            return resp;
        }

        // 增加屏蔽词过滤
        if(msg.find("shit") != std::string::npos)
        {
            resp["Result"] = false;
            resp["Reason"] = "There are blocked words in the field, please edit again before sending.";
            return resp;
        }
        
        // 聊天信息广播
        resp["Result"] = "true";
        resp["Reason"] = "Chat";
        return resp;
    }
    

};

#endif // _ROOM_HPP_