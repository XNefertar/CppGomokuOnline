#ifndef _MATCHER_HPP_
#define _MATCHER_HPP_

#include <thread>
#include "Log.hpp"
#include "MatchQueue.hpp"
#include "RoomManage.hpp"
#include "UserTable.hpp"
#include "OnlineManage.hpp"

#define LEVEL_NORMAL 0
#define LEVEL_HIGH   1
#define LEVEL_SUPER  2
#define LEVEL_TYPE   int

#define MAPPING_TO_LEVEL(LEVEL_SCORE, LEVEL) do{\
    if(LEVEL_SCORE < 1000)\
    {\
        LEVEL = LEVEL_NORMAL;\
    }\
    else if(LEVEL_SCORE < 2000 && LEVEL_SCORE >= 1000)\
    {\
        LEVEL = LEVEL_HIGH;\
    }\
    else\
    {\
        LEVEL = LEVEL_SUPER;\
    }\
}while(0)

class Matcher
{
private:
    // 根据天梯分划分匹配等级
    MatchQueue<uint64_t> _QueueNormal;
    MatchQueue<uint64_t> _QueueHigh;
    MatchQueue<uint64_t> _QueueSuper;
    // 匹配线程
    // 分别处理不同等级的匹配
    std::thread _NormalThread;
    std::thread _HighThread;
    std::thread _SuperThread;
    // 房间管理类
    RoomManage *_RoomManage;
    UserTable *_UserTable;
    OnlineManage *_OnlineUser;

public:

    Matcher(RoomManage *RoomManage, UserTable *UserTable, OnlineManage *OnlineUser)
        : _RoomManage(RoomManage),
          _UserTable(UserTable),
          _OnlineUser(OnlineUser)
    {
        _NormalThread = std::thread(&Matcher::HandlerMatch, this, std::ref(_QueueNormal));
        _HighThread = std::thread(&Matcher::HandlerMatch, this, std::ref(_QueueHigh));
        _SuperThread = std::thread(&Matcher::HandlerMatch, this, std::ref(_QueueSuper));
        Log::LogInit("log.txt", NORMAL);
        Log::LogMessage(INFO, "Matcher init success");
    }

    ~Matcher()
    {
        if (_NormalThread.joinable())  { _NormalThread.join(); }
        if (_HighThread.joinable())    { _HighThread.join();   }
        if (_SuperThread.joinable())   { _SuperThread.join();  }
    }

    void AddMatch(uint64_t uid)
    {
        Json::Value user;
        if (!_UserTable->SelectByUID(uid, user))
        {
            return;
        }
        LEVEL_TYPE score = user["score"].asInt();
        LEVEL_TYPE level = 0;
        MAPPING_TO_LEVEL(score, level);
        switch (level)
        {
        case 0:
            _QueueNormal.Push(uid);
            break;
        case 1:
            _QueueHigh.Push(uid);
            break;
        case 2:
            _QueueSuper.Push(uid);
            break;
        default:
            break;
        }
    }

    bool DelMatch(uint64_t uid)
    {
        Json::Value user;
        if (!_UserTable->SelectByUID(uid, user))
        {
            return false;
        }
        LEVEL_TYPE score = user["score"].asInt();
        LEVEL_TYPE level = 0;
        MAPPING_TO_LEVEL(score, level);
        switch (level)
        {
        case 0:
            _QueueNormal.Remove(uid);
            break;
        case 1:
            _QueueHigh.Remove(uid);
            break;
        case 2:
            _QueueSuper.Remove(uid);
            break;
        default:
            break;
        }
        return true;
    }


private:
    void HandlerMatch(MatchQueue<uint64_t> &mQueue)
    {
        while (1)
        {
            while (mQueue.Size() < 2)
            {
                mQueue.WaitThread();
            }

            uint64_t uid1 = mQueue.Pop();
            uint64_t uid2 = mQueue.Pop();
            RoomPtr room = _RoomManage->CreateRoom(uid1, uid2);
            if (room == nullptr)
            {
                continue;
            }
            // 如果匹配成功，通知双方玩家
            // 如果某一方掉线，将队列中的下一位玩家加入房间
            wsserver_t::connection_ptr WebConPtr1;
            _OnlineUser->GetGameHallConnection(uid1, WebConPtr1);
            if (WebConPtr1 == nullptr)
            {
                this->AddMatch(uid2);
                continue;
            }

            wsserver_t::connection_ptr WebConPtr2;
            _OnlineUser->GetGameHallConnection(uid2, WebConPtr2);
            if (WebConPtr2 != nullptr)
            {
                this->AddMatch(uid1);
                continue;
            }

            // ???
            RoomPtr room = _RoomManage->CreateRoom(uid1, uid2);
            if (room == nullptr)
            {   
                this->AddMatch(uid1);
                this->AddMatch(uid2);
                continue;
            }

            // 通知双方玩家
            Json::Value resp;
            resp["RoomID"] = room->GetRoomId();
            resp["OpType"] = "MatchSuccess";
            resp["Result"] = "true";
            resp["Reason"] = "Match success";
            std::string str;
            JsonCpp::serialize(resp, str);
            WebConPtr1->send(str);
            WebConPtr2->send(str);
        }
    }
    void NormalThreadEntry() { HandlerMatch(_QueueNormal); }
    void HighThreadEntry() { HandlerMatch(_QueueHigh); }
    void SuperThreadEntry() { HandlerMatch(_QueueSuper); }
};

#endif // _MATCHER_HPP_