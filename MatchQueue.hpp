#ifndef _MATCH_QUEUE_HPP_
#define _MATCH_QUEUE_HPP_

#define QUEUE_SIZE_TYPE int

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
        _Cond.notify_one();
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


};


#endif // _MATCH_QUEUE_HPP_