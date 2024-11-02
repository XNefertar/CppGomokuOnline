#ifndef _SESSION_MANAGE_HPP_
#define _SESSION_MANAGE_HPP_

#include "Session.hpp"
#include "Logger.hpp"
#include <unordered_map>

#define SESSION_TIMEOUT 30000
#define SESSION_FOREVER -1

using SessionPtr = std::shared_ptr<Session>;


class SessionManage
{
private:
    uint64_t                                _NextSID;
    std::mutex                              _Mutex;
    std::unordered_map<uint64_t, SessionPtr> _SessionMap;
    wsserver_t                              *_Server;

public:
    SessionManage(wsserver_t *server)
        : _NextSID(1),
          _Server(server)
    {
        DBG_LOG("SessionManage init success");
        // Log::LogMessage(INFO, "SessionManage init success");
    }

    ~SessionManage()
    {
        DBG_LOG("SessionManage destroy");
        // Log::LogMessage(INFO, "SessionManage destroy");
    }

    SessionPtr CreateSession(uint64_t uid, SESSION_STATE state)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        SessionPtr sessionPtr(new Session(_NextSID));
        sessionPtr->SetState(state);
        sessionPtr->SetUID(uid);
        _SessionMap.insert(std::make_pair(_NextSID, sessionPtr));
        ++_NextSID;

        DBG_LOG("Create Session Success: %lu", sessionPtr->GetSessionID());

        return sessionPtr;
    }

    void AppendSession(const SessionPtr &sessionPtr)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        _SessionMap.insert(std::make_pair(sessionPtr->GetSessionID(), sessionPtr));
    }

    SessionPtr GetSessionBySID(uint64_t sid)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        auto iter = _SessionMap.find(sid);
        if(iter == _SessionMap.end())
        {
            return SessionPtr();
        }
        return iter->second;
    }

    void RemoveSession(uint64_t sid)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        _SessionMap.erase(sid);
    }


    // 情形 1：如果定时器为空且 ms 为 SESSION_FOREVER，表示会话应该永久存在，直接返回。
    // 情形 2：如果定时器为空且 ms 不是 SESSION_FOREVER，则设置一个新的定时器，用于在指定时间后删除会话。
    // 情形 3：如果定时器不为空且 ms 为 SESSION_FOREVER，则取消当前定时器，并将会话设置为永久存在，且将会话的定时器设置为空。
    // 情形 4：如果定时器不为空且 ms 不是 SESSION_FOREVER，取消当前定时器，并设置会话的新定时器以重置过期时间。

    // Bind Function 
    // TODO
    void SetSessionExpireTime(uint64_t sid, int time)
    {
        SessionPtr sessionPtr = GetSessionBySID(sid);
        if(sessionPtr.get() == nullptr)
        {
            return;
        }


        // 获取定时器
        wsserver_t::timer_ptr timer = sessionPtr->GetTimer();
        if(timer.get() == nullptr && time == SESSION_FOREVER)
        {
            return;
        }

        else if(timer.get() == nullptr && time != SESSION_FOREVER)
        {
            wsserver_t::timer_ptr new_timer = _Server->set_timer(time, std::bind(&SessionManage::RemoveSession, this, sid));
            sessionPtr->SetTimer(new_timer);
        }

        else if(timer.get() != nullptr && time == SESSION_FOREVER)
        {
            timer->cancel();
            sessionPtr->SetTimer(wsserver_t::timer_ptr());
            _Server->set_timer(0, std::bind(&SessionManage::AppendSession, this, sessionPtr));
        }

        else if(timer.get() != nullptr && time != SESSION_FOREVER)
        {
            timer->cancel();
            sessionPtr->SetTimer(wsserver_t::timer_ptr());
            _Server->set_timer(0, std::bind(&SessionManage::AppendSession, this, sessionPtr));
            wsserver_t::timer_ptr new_timer = _Server->set_timer(time, std::bind(&SessionManage::RemoveSession, this, sessionPtr->GetSessionID()));
            sessionPtr->SetTimer(new_timer);
        }
    }
};

#endif // _SESSION_MANAGE_HPP_