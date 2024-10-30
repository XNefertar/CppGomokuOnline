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
    user_table   *_UserTable;        // 用户列表  TODO 未实现 MySQL
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
    Room(uint64_t RoomId, user_table *UserTable, OnlineManage *OnlineUser)
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
    

};




#endif // _ROOM_HPP_