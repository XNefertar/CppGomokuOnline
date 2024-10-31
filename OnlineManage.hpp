#ifndef _ONLINE_MANAGE_HPP_
#define _ONLINE_MANAGE_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <jsoncpp/json/json.h>
#include "Log.hpp"

typedef websocketpp::server<websocketpp::config::asio> wsserver_t;

class OnlineManage
{
private:
    std::unordered_map<uint16_t, wsserver_t::connection_ptr> _GameHall;
    std::unordered_map<uint16_t, wsserver_t::connection_ptr> _GameRoom;
    std::mutex _mutex;

public:
    // 进入游戏大厅和游戏房间
    void EnterGameHall(uint16_t id, wsserver_t::connection_ptr &con) {
        std::lock_guard<std::mutex> lock(_mutex);
        _GameHall[id] = con;
    }
    void EnterGameRoom(uint16_t id, wsserver_t::connection_ptr &con) {
        std::lock_guard<std::mutex> lock(_mutex);
        _GameRoom[id] = con;
    }


    // 退出游戏大厅和游戏房间
    void ExitGameHall(uint16_t id, wsserver_t::connection_ptr hdl) {
        std::lock_guard<std::mutex> lock(_mutex);
        _GameHall.erase(id);
    }
    void ExitGameRoom(uint16_t id) {
        std::lock_guard<std::mutex> lock(_mutex);
        _GameRoom.erase(id);
    }


    // 判断是否在游戏大厅和游戏房间
    bool IsInGameHall(uint16_t id) {
        std::lock_guard<std::mutex> lock(_mutex);
        return _GameHall.find(id) != _GameHall.end();
    }
    bool IsInGameRoom(uint16_t id) {
        std::lock_guard<std::mutex> lock(_mutex);
        return _GameRoom.find(id) != _GameRoom.end();
    }


    // 获取游戏大厅和游戏房间的连接
    bool GetGameHallConnection(uint16_t id, wsserver_t::connection_ptr& hdl) {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_GameHall.find(id) == _GameHall.end()) 
        {
            return false;
        }
        hdl = _GameHall.at(id);
        return true;
    }
    bool GetGameRoomConnection(uint16_t id, wsserver_t::connection_ptr& hdl) {
        std::lock_guard<std::mutex> lock(_mutex);
        if(_GameRoom.find(id) == _GameRoom.end()) 
        {
            return false;
        }
        hdl = _GameRoom.at(id);
        return true;
    }
};;


#endif // _ONLINE_MANAGE_HPP_