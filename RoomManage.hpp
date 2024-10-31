#ifndef _ROOM_MANAGE_HPP_
#define _ROOM_MANAGE_HPP_

#include "Room.hpp"
#include "OnlineManage.hpp"
#include "UserTable.hpp"
#include "Log.hpp"
#include "MySQL.hpp"
#include "JsonCpp.hpp"

using namespace LOG_MSG;
using RoomPtr = std::shared_ptr<Room>;

class RoomManage
{
private:
    uint64_t                                _NextRID;
    std::mutex                              _Mutex;
    UserTable                               *_UserTable;
    OnlineManage                            *_OnlineUser;
    // 房间ID和房间指针的映射
    std::unordered_map<uint64_t, RoomPtr>   _RoomMap;
    // 用户ID和房间ID的映射
    std::unordered_map<uint64_t, uint64_t>  _Users;

public:
    RoomManage(UserTable *UserTable, OnlineManage *OnlineUser)
        : _NextRID(0),
          _UserTable(UserTable),
          _OnlineUser(OnlineUser)
    {
        Log::LogInit("log.txt", NORMAL);
        Log::LogMessage(INFO, "RoomManage init success");
    }
    ~RoomManage()
    {
        Log::LogMessage(INFO, "RoomManage destroy");
    }

    RoomPtr CreateRoom(uint64_t uid1, uint64_t uid2)
    {
        // 校验用户是否在线
        if(!_OnlineUser->IsInGameHall(uid1) || !_OnlineUser->IsInGameHall(uid2))
        {
            Log::LogMessage(ERROR, "User is not online");
            return nullptr;
        }

        std::lock_guard<std::mutex> lock(_Mutex);
        RoomPtr room = std::make_shared<Room>(_NextRID, _UserTable, _OnlineUser);
        room->AddWhitePlayer(uid1);
        room->AddBlackPlayer(uid2);
        _RoomMap.insert(std::make_pair(room->GetRoomId(), room));
        _Users.insert(std::make_pair(uid1, room->GetRoomId()));
        _Users.insert(std::make_pair(uid2, room->GetRoomId()));
        ++_NextRID;

        return room;
    }

    RoomPtr GetRoomByRID(uint64_t rid)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        auto iter = _RoomMap.find(rid);
        if(iter == _RoomMap.end())
        {
            return nullptr;
        }
        return iter->second;
    }

    RoomPtr GetRoomByUID(uint64_t uid)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        auto UserIter = _Users.find(uid);
        if(UserIter == _Users.end())
        {
            return nullptr;
        }
        // 增加指针正确性校验
        // return GetRoomByRID(iter->second);
        auto RoomIter = _RoomMap.find(UserIter->second);
        if(RoomIter == nullptr)
        {
            return nullptr;
        }
        return RoomIter->second;
    }

    void RemoveRoom(uint64_t rid)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        auto iter = _RoomMap.find(rid);
        if(iter == _RoomMap.end())
        {
            return;
        }
        _RoomMap.erase(iter);
    }

    void RemoveRoomByUID(uint64_t uid)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        RoomPtr room = GetRoomByUID(uid);
        if(room == nullptr)
        {
            return;
        }
        // 处理玩家退出
        room->HandlerQuit(uid);
        // 如果房间人数为0，移除房间
        if(room->GetPlayerNum() == 0)
        {
            RemoveRoom(room->GetRoomId());
        }
        return;
    }

    

};




#endif // _ROOM_MANAGE_HPP_