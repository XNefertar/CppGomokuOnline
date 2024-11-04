#ifndef _ROOM_MANAGE_HPP_
#define _ROOM_MANAGE_HPP_

#include "Room.hpp"
#include "OnlineManage.hpp"
#include "UserTable.hpp"
#include "Logger.hpp"
#include "MySQL.hpp"
#include "JsonCpp.hpp"

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
        : _NextRID(1),
          _UserTable(UserTable),
          _OnlineUser(OnlineUser)
    {
        DBG_LOG("RoomManage init success");
        // Log::LogMessage(INFO, "RoomManage init success");
    }
    ~RoomManage()
    {
        // Log::LogMessage(INFO, "RoomManage destroy");
    }

    RoomPtr CreateRoom(uint64_t uid1, uint64_t uid2)
    {
        // 校验用户是否在线
        if(!_OnlineUser->IsInGameHall(uid1) || !_OnlineUser->IsInGameHall(uid2))
        {
            ERR_LOG("User is not online");
            // Log::LogMessage(ERROR, "User is not online");
            return RoomPtr();
        }

        std::lock_guard<std::mutex> lock(_Mutex);
        RoomPtr room(new Room(_NextRID, _UserTable, _OnlineUser));
        room->AddWhitePlayer(uid1);
        room->AddBlackPlayer(uid2);
        _RoomMap.insert(std::make_pair(_NextRID, room));
        _Users.insert(std::make_pair(uid1, _NextRID));
        _Users.insert(std::make_pair(uid2, _NextRID));
        ++_NextRID;

        DBG_LOG("Create Room Success");
        return room;
    }

    RoomPtr GetRoomByRID(uint64_t rid)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        auto iter = _RoomMap.find(rid);
        if(iter == _RoomMap.end())
        {
            return RoomPtr();
        }
        return iter->second;
    }

    RoomPtr GetRoomByUID(uint64_t uid)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        auto UserIter = _Users.find(uid);
        if(UserIter == _Users.end())
        {
            return RoomPtr();
        }
        // 增加指针正确性校验
        // return GetRoomByRID(iter->second);
        auto RoomIter = _RoomMap.find(UserIter->second);
        if(RoomIter == _RoomMap.end())
        {
            return RoomPtr();
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