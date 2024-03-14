/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-03-01 02:20:34
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-03 21:52:36
 */
#include "fiber.hpp"
#include "scheduler.hpp"

static MySylar::Logger::loggerPtr _logger = LOG_ROOT();

void TestFunc1() {
    INFO_LOG(_logger) << "TestFunc1 begin()";

    //协程主动让出执行权，在yield之前，协程必须再次将自己添加到调度器任务队列中
    //否则回跑飞
    MySylar::Scheduler::GetThis()->Schedule(MySylar::Fiber::GetThis());

    INFO_LOG(_logger) << "before TestFunc1 yield";
    MySylar::Fiber::GetThis()->Yield();
    INFO_LOG(_logger) << "after TestFunc1 yield";

    INFO_LOG(_logger) << "TestFunc1 end";
}

void TestFunc2() {
    INFO_LOG(_logger) << "test_fiber2 begin";

    /**
     * 一个线程同一时间只能有一个协程在运行，线程调度协程的本质就是按顺序执行任务队列里的协程
     * 由于必须等一个协程执行完后才能执行下一个协程，所以任何一个协程的阻塞都会影响整个线程的协程调度，这里
     * 睡眠的3秒钟之内调度器不会调度新的协程，后续改变这种情况
     */
    sleep(3);

    INFO_LOG(_logger) << "test_fiber2 end";
}

void TestFunc3() {
    INFO_LOG(_logger) << "test_fiber3 begin";
    INFO_LOG(_logger) << "test_fiber3 end";
}

void TestFunc5() {
    static int count = 0;

    INFO_LOG(_logger) << "test_fiber5 begin, i = " << count;
    INFO_LOG(_logger) << "test_fiber5 end i = " << count;

    count++;
}

void TestFunc4() {
    INFO_LOG(_logger) << "test_fiberaa begin";
    
    for (int i = 0; i < 3; i++) {
        MySylar::Scheduler::GetThis()->Schedule(TestFunc5, MySylar::GetThreadId());
    }

    INFO_LOG(_logger) << "test_fiber4 end";
}

int main(void) {

    MySylar::FileLogAppender::logAppenderPtr logFile(new MySylar::FileLogAppender("./log.txt"));
    _logger->AddAppender(logFile);
    _logger->SetLevel(MySylar::LogLevel::DEBUG);

    INFO_LOG(_logger) << "main start";

    ///使用ain函数线程进行协程调度，相当于先攒下一波协程，然后切换到调度器的run方法将这些协程消耗掉
    ///然后再返回mmain函数往下执行
    MySylar::Scheduler sc;

    // 额外创建新的线程进行的调度，那只要添加了调度任务，调度器马上就可以调度该任务
    ///MySylar::Schedular sc(3, false);

    sc.Schedule(TestFunc1);
    sc.Schedule(TestFunc2);

    //fiber类作为调度对象
    MySylar::Fiber::FiberPtr fiber(new MySylar::Fiber(&TestFunc3));
    sc.Schedule(fiber);

    //main函数线程进行调度，那什么也没做
    sc.Start();

    //调度器未停止，就可以添加调度任务
    //子协程也可以添加调度任务
    sc.Schedule(TestFunc4);

    //停止调度，如果未使用当前线程进行调度，那么只需要简单地等所有调度线程退出即可
    //如果使用了当前线程进行调度，那么要先执行当前线程地协程调度函数，等其执行完后再返回caller协程继续往下执行
    sc.Stop();

    
    
    return 0;
}