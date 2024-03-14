/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-14 01:08:14
 * @LastEditors: LOSER
 * @LastEditTime: 2024-02-15 21:50:26
 */
#include "thread.hpp"
#include "log.hpp"
#include "other.hpp"

namespace MySylar{

static thread_local Thread * _aThread = nullptr;
static thread_local std::string _aThreadName = "UNKNOW";

static Logger::loggerPtr _threadLogger = LOG_NAME("SYSTEM");

Thread *Thread::GetThreadThis() {
    return _aThread;
}

const std::string &Thread::CurGetName() {
    return _aThreadName;
}

void Thread::SetThreadName(const std::string &name) {
    if (name.empty()) {
        return;
    }
    if (_aThread) {
        _aThread->_tName = name;
    }
    _aThreadName = name;
}


MySylar::Thread::Thread(std::function<void()> cb, const std::string &name) 
:_tCb(cb)
,_tName(name){
    if(name.empty()) {
        _tName = "UNKNOW";
    }

    int rtval = pthread_create(&_thread, nullptr, &Thread::run, this);
    if(rtval) {
        ERROR_LOG(_threadLogger) << "pthread_create error return val = " << rtval
        << " name=" << name;

        throw std::logic_error("pthread_create error");
    }
    
    _threadSp.wait();
}

Thread::~Thread()
{
    if(_thread) {
        pthread_detach(_thread);
    }
}

void Thread::Join()
{
    if(_thread) {
        int retval = pthread_join(_thread, nullptr);
        if(retval) {
            ERROR_LOG(_threadLogger) << "pthread_join error return val = " << retval;

            throw std::logic_error("pthread_join error");
        }
        _thread = 0;
    }

}
void *Thread::run(void *arg)
{
    Thread * tmpThread = (Thread*) arg;
    _aThread = tmpThread;
    _aThreadName = tmpThread->_tName;
    tmpThread->_pid = GetThreadId();
    pthread_setname_np(pthread_self(), tmpThread->_tName.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(tmpThread->_tCb);

    tmpThread->_threadSp.notify();
    cb();
    return 0;
}
}
