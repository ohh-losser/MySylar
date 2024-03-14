/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-14 01:08:20
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-01 17:30:23
 */

#ifndef __THREAD__HPP__
#define __THREAD__HPP__


#include "mutex.hpp"

namespace MySylar{

class Thread : Nocopyable {
public:
    using ThreadPtr = std::shared_ptr<Thread>;

public:
    Thread(std::function<void()> cb, const std::string &name);

    ~Thread();

    pid_t GetId() const { return _pid;}

    const std::string & GetName() const {return _tName;}

    void Join();

    static Thread * GetThreadThis();

    static const std::string &CurGetName();

    static void SetThreadName(const std::string & name);

private:
    static void * run(void * arg);
    
private:
    // 线程id
    pid_t _pid = -1;
    // 线程结构体
    pthread_t _thread = 0;
    // 线程执行函数
    std::function<void()> _tCb;
    // 线程名称
    std::string _tName;
    // 信号量
    Semaphore _threadSp;
};

}

#endif