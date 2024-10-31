#ifndef _SESSION_HPP_
#define _SESSION_HPP_


#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <string>
#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include "Log.hpp"

using namespace LOG_MSG;
typedef websocketpp::server<websocketpp::config::asio> wsserver_t;
typedef enum
{
    UNLOGIN,
    LOGIN,
}SESSION_STATE;

class Session
{
private:
    uint64_t                _UID;
    uint64_t                _SessionID;
    SESSION_STATE           _State;
    wsserver_t::timer_ptr   _TimerPtr;

public:
    Session(uint64_t uid, uint64_t sid)
        : _UID(uid),
          _SessionID(sid),
          _State(UNLOGIN),
          _TimerPtr(nullptr)
    {
        // Log::LogMessage(INFO, "Session init success");
    }

    ~Session()
    {
        if(_TimerPtr != nullptr)
        {
            _TimerPtr->cancel();
            // Log::LogMessage(INFO, "Session destroy");
        }
    }

    uint64_t GetUID() { return _UID; }
    uint64_t GetSessionID() { return _SessionID; }
    SESSION_STATE GetState() { return _State; }
    void SetState(SESSION_STATE state) { _State = state; }
    void SetTimer(const wsserver_t::timer_ptr timer) { _TimerPtr = timer; }
    void SetUID(uint64_t uid) { _UID = uid; }
    bool IsLogIn() { return _State == LOGIN; }
    wsserver_t::timer_ptr& GetTimer() { return _TimerPtr; }

    void CancelTimer()
    {
        if (_TimerPtr != nullptr)
        {
            _TimerPtr->cancel();
        }
    }
};


#endif // _SESSION_HPP_