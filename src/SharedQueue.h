// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>

#ifndef QUEUE_SHARED_MUTEX__SHAREDQUEUE_H_
#    define QUEUE_SHARED_MUTEX__SHAREDQUEUE_H_
using namespace std;

template <typename T>
class SharedQueue
{
public:
    queue<T> q;
    std::mutex mutex_;
    std::condition_variable cv;
    bool use_mutex = true;
    SharedQueue() = default;

    void justNotify() { cv.notify_one(); }

    void push(T i)
    {
        if (use_mutex)
        {
            std::lock_guard lock(mutex_);
        }
        this->q.push(i);
        if (use_mutex)
        {
            cv.notify_one();
        }
    }

    T pop()
    {
        std::unique_lock lock(mutex_);
        while (q.empty())
        {
            cv.wait(lock);
        }

        auto result = move(q.front());
        q.pop();
        return result;
    }

    T popIfExist()
    {
        std::unique_lock lock(mutex_);
        if (q.empty())
        {
            return nullptr;
        }
        auto result = move(q.front());
        q.pop();
        return result;
    }

    int getSize()
    {
        std::lock_guard lock(mutex_);
        return q.size();
    }
    ~SharedQueue() = default;
};

#endif // QUEUE_SHARED_MUTEX__SHAREDQUEUE_H_
