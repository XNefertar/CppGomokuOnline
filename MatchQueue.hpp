#ifndef _MATCH_QUEUE_HPP_
#define _MATCH_QUEUE_HPP_

#define QUEUE_SIZE_TYPE int
#include <queue>
#include <condition_variable>
#include "Logger.hpp"
template<class T>
class MatchQueue
{
    typedef T ValueType;
    typedef QUEUE_SIZE_TYPE size_type;
    typedef T& Reference;
    typedef const T& ConstReference;
    typedef T* Pointer;
    typedef const T* ConstPointer;

private:
    std::queue<T> _Queue;
    std::mutex    _Mutex;
    std::condition_variable _Cond;

public:
    QUEUE_SIZE_TYPE Size()
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        return _Queue.size();
    }

    bool IsEmpty()
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        return _Queue.empty();
    }

    void WaitThread()
    {
        std::unique_lock<std::mutex> lock(_Mutex);
        _Cond.wait(lock);
    }

    void Push(const T &t)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        _Queue.push(t);
        _Cond.notify_all();
    }

    ValueType Pop()
    {
        std::unique_lock<std::mutex> lock(_Mutex);
        while(_Queue.empty())
        {
            _Cond.wait(lock);
        }
        ValueType t = _Queue.front();
        _Queue.pop();
        return t;
    }

    bool Pop(ValueType &data)
    {   
        std::unique_lock<std::mutex> lock(_Mutex);
        if(_Queue.empty())
        {
            ERR_LOG("Queue is empty");
            return false;
        }
        data = _Queue.front();
        _Queue.pop();
        DBG_LOG("Pop Data: %d", data);
        return true;
    }

    void Remove(ValueType& t)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        std::queue<T> tmp;
        while(!_Queue.empty())
        {
            if(_Queue.front() != t)
            {
                tmp.push(_Queue.front());
            }
            _Queue.pop();
        }
        _Queue = tmp;
    }

    void Print()
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        while(!_Queue.empty())
        {
            INF_LOG("Queue Data: %d", _Queue.front());
            _Queue.pop();
        }
    }

    void Clear()
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        while(!_Queue.empty())
        {
            _Queue.pop();
        }
    }


};


#endif // _MATCH_QUEUE_HPP_