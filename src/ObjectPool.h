// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef CLANG_TIDY_SRC_OBJECTPOOL_H_
#define CLANG_TIDY_SRC_OBJECTPOOL_H_

#include <iostream>
#include <set>
#include <stack>
#include <stdexcept>
using namespace std;

template <class T>
class ObjectPool
{
public:
    T * Allocate()
    {
        this->mutex.lock();
        if (this->pool.empty())
        {
            return new T;
        }
        T * i = this->pool.top();
        this->pool.pop();
        this->mutex.unlock();
        return i;
    };

    void Deallocate(T * object)
    {
        this->mutex.lock();
        this->pool.push(object);
        this->mutex.unlock();
    };

    ~ObjectPool(){};

private:
    std::mutex mutex;
    stack<T *> pool;
};

#endif //CLANG_TIDY_SRC_OBJECTPOOL_H_
