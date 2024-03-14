/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-21 19:52:40
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-02 21:59:20
 */
#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>
#include <list>

#include "nocopyable.hpp"

namespace MySylar {

class Semaphore : Nocopyable {
public:
    /**
     * @brief 构造函数
     * @param[in] count 信号量值的大小
     */
    Semaphore(uint32_t count = 0) {
        if(sem_init(&_semaphore, 0, count)) {
            throw std::logic_error("sem_init error");
        }
    }

    /**
     * @brief 析构函数
     */
    ~Semaphore() {
        sem_destroy(&_semaphore);
    }

    /**
     * @brief 获取信号量
     */
    void wait() {
        if(sem_wait(&_semaphore)) {
            throw std::logic_error("sem_wait error");
        }
    }

    /**
     * @brief 释放信号量
     */
    void notify() {
        if(sem_post(&_semaphore)) {
            throw std::logic_error("sem_post error");
        }
    }
private:
    sem_t _semaphore;
};


/**
 * @brief 局部锁的模板实现
 */
template<class T>
struct ScopedLockImpl {
public:
    /**
     * @brief 构造函数
     * @param[in] mutex Mutex
     */
    ScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.Lock();
        m_locked = true;
    }

    /**
     * @brief 析构函数,自动释放锁
     */
    ~ScopedLockImpl() {
        Unlock();
    }

    /**
     * @brief 加锁
     */
    void Lock() {
        if(!m_locked) {
            m_mutex.Lock();
            m_locked = true;
        }
    }

    /**
     * @brief 解锁
     */
    void Unlock() {
        if(m_locked) {
            m_mutex.Unlock();
            m_locked = false;
        }
    }
private:
    /// mutex
    T& m_mutex;
    /// 是否已上锁
    bool m_locked;
};

/**
 * @brief 局部读锁模板实现
 */
template<class T>
struct ReadScopedLockImpl {
public:
    /**
     * @brief 构造函数
     * @param[in] mutex 读写锁
     */
    ReadScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.RdLock();
        m_locked = true;
    }

    /**
     * @brief 析构函数,自动释放锁
     */
    ~ReadScopedLockImpl() {
        Unlock();
    }

    /**
     * @brief 上读锁
     */
    void Lock() {
        if(!m_locked) {
            m_mutex.RdLock();
            m_locked = true;
        }
    }

    /**
     * @brief 释放锁
     */
    void Unlock() {
        if(m_locked) {
            m_mutex.Unlock();
            m_locked = false;
        }
    }
private:
    /// mutex
    T& m_mutex;
    /// 是否已上锁
    bool m_locked;
};
/**
 * @brief 局部写锁模板实现
 */
template<class T>
struct WriteScopedLockImpl {
public:
    /**
     * @brief 构造函数
     * @param[in] mutex 读写锁
     */
    WriteScopedLockImpl(T& mutex)
        :m_mutex(mutex) {
        m_mutex.WrLock();
        m_locked = true;
    }

    /**
     * @brief 析构函数
     */
    ~WriteScopedLockImpl() {
        Unlock();
    }

    /**
     * @brief 上写锁
     */
    void Lock() {
        if(!m_locked) {
            m_mutex.WrLock();
            m_locked = true;
        }
    }

    /**
     * @brief 解锁
     */
    void Unlock() {
        if(m_locked) {
            m_mutex.Unlock();
            m_locked = false;
        }
    }
private:
    /// Mutex
    T& m_mutex;
    /// 是否已上锁
    bool m_locked;
};
/**
 * @brief 互斥量
 */
class Mutex : Nocopyable {
public: 
    /// 局部锁
    typedef ScopedLockImpl<Mutex> lock;

    /**
     * @brief 构造函数
     */
    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    /**
     * @brief 析构函数
     */
    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    /**
     * @brief 加锁
     */
    void Lock() {
        pthread_mutex_lock(&m_mutex);
    }

    /**
     * @brief 解锁
     */
    void Unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    /// mutex
    pthread_mutex_t m_mutex;
};

/**
 * @brief 自旋锁
 */
class Spinlock : Nocopyable {
public:
    /// 局部锁
    typedef ScopedLockImpl<Spinlock> lock;

    /**
     * @brief 构造函数
     */
    Spinlock() {
        pthread_spin_init(&m_mutex, 0);
    }

    /**
     * @brief 析构函数
     */
    ~Spinlock() {
        pthread_spin_destroy(&m_mutex);
    }

    /**
     * @brief 上锁
     */
    void Lock() {
        pthread_spin_lock(&m_mutex);
    }

    /**
     * @brief 解锁
     */
    void Unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    /// 自旋锁
    pthread_spinlock_t m_mutex;
};

/**
 * @brief 读写互斥量
 */
class RWMutex : Nocopyable{
public:

    /// 局部读锁
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    /// 局部写锁
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    /**
     * @brief 构造函数
     */
    RWMutex() {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    
    /**
     * @brief 析构函数
     */
    ~RWMutex() {
        pthread_rwlock_destroy(&m_lock);
    }

    /**
     * @brief 上读锁
     */
    void RdLock() {
        pthread_rwlock_rdlock(&m_lock);
    }

    /**
     * @brief 上写锁
     */
    void WrLock() {
        pthread_rwlock_wrlock(&m_lock);
    }

    /**
     * @brief 解锁
     */
    void Unlock() {
        pthread_rwlock_unlock(&m_lock);
    }
private:
    /// 读写锁
    pthread_rwlock_t m_lock;
};


}

#endif 
