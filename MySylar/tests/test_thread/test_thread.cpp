/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-15 21:12:20
 * @LastEditors: LOSER
 * @LastEditTime: 2024-02-15 23:13:41
 */

#include "log.hpp"
#include "env.hpp"
#include "config.hpp"
#include "thread.hpp"
#include "mutex.hpp"

static MySylar::Logger::loggerPtr testThreadLogger = LOG_ROOT(); 

int count = 0;
MySylar::Mutex _testMutex;

void func(void *arg) {
    DEBUG_LOG(testThreadLogger) << "name:" << MySylar::Thread::CurGetName()
        << " this.name:" << MySylar::Thread::GetThreadThis()->GetName()
        << " thread name:" << MySylar::GetThreadName()
        << " id:" << MySylar::GetThreadId()
        << " this.id:" << MySylar::Thread::GetThreadThis()->GetId();
    DEBUG_LOG(testThreadLogger) << "arg: " << *(int*)arg;
    for(int i = 0; i < 10000; i++) {
        MySylar::Mutex::lock _lock(_testMutex);
        ++count;
    }
}


int main(int argc, char *argv[]) {
    MySylar::singelEnv::GetInstance()->Init(argc, argv);
    MySylar::Config::LoadFromConfDir(MySylar::singelEnv::GetInstance()->GetConfigPath());
    
    testThreadLogger->SetLevel(MySylar::LogLevel::Level::DEBUG);

    std::vector<MySylar::Thread::ThreadPtr> thrs;
    int arg = 123456;
    for(int i = 0; i < 3; i++) {
        MySylar::Thread::ThreadPtr test_thread(new MySylar::Thread(std::bind(func, &arg), "thread_" + std::to_string(i)));
    }
    
}