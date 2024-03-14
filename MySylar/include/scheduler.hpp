/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-24 10:53:16
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-02 21:59:36
 */
#ifndef __SCHEDULER_HPP__
#define __SCHEDULER_HPP__

#include "log.hpp"
#include "fiber.hpp"
#include "thread.hpp"



namespace MySylar
{

class Scheduler {
public:
    using schedularPtr = std::shared_ptr<Scheduler>;
    using schedularMutex =  Mutex;

public:
    Scheduler(size_t threads = 1, bool use_caller = true, const std::string & name = "Scheduler");

    virtual ~Scheduler();

    const std::string & GetName() const {return _name;}

    /// @brief 添加调度任务
    /// @tparam FiberOrCb 调度任务类型， 可以是协程对象或函数指针
    /// @param fc 协程对象或指针
    /// @param thread 指定运行该任务的线程号， -1表示任意线程
    template <class FiberOrCb>
    void Schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            schedularMutex::lock lock(_mutex);
            need_tickle = ScheduleNoLock(fc, thread);
        }

        if(need_tickle) {
            Tickle();
        }
    }

    void Start();

    void Stop();

protected:
    /// @brief 通知协程调度器有任务
    virtual void Tickle();

    /// @brief 协程调度函数
    void Run();

    /// @brief 无任务时执行
    virtual void Idle();

    /// @brief 是否可以停止
    virtual bool Stopping();

    /// @brief 设置当前的协程调度器
    void SetThis();
    
    /// @brief 当调度线程进入idle时空闲线程数加1， 从idle协程返回时空闲线程数减1
    /// @return 返回是否有空闲线程
    bool HasIdleThreads(){return _idleThreadCount > 0;}


private:
        /**
     * @brief 添加调度任务，无锁
     * @tparam FiberOrCb 调度任务类型，可以是协程对象或函数指针
     * @param[] fc 协程对象或指针
     * @param[] thread 指定运行该任务的线程号，-1表示任意线程
     */
    template <class FiberOrCb>
    bool ScheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = _tasks.empty();
        ScheduleTask task(fc, thread);
        if(task._fiber || task._cb) {
            _tasks.push_back(task);
        }

        return need_tickle;
    }

public:
    static Scheduler * GetThis();

    static Fiber * GetMainFiber();    
private:
    /**
     * @brief 调度任务，协程/函数二选一，可指定在哪个线程上调度
     */
    struct ScheduleTask {
        Fiber::FiberPtr _fiber;

        std::function<void()> _cb;
        int _thread;

        ScheduleTask(Fiber::FiberPtr fiber,int thread) {
            _fiber = fiber;
            _thread = thread;
        }

        ScheduleTask(Fiber::FiberPtr * fiber, int thread) {
            _fiber.swap(*fiber);
            _thread = thread;
        }

        ScheduleTask(std::function<void()> cb, int thread) {
            _cb = cb;
            _thread = thread;
        }

        ScheduleTask() { _thread = -1;}

        void reset() {
            _fiber = nullptr;
            _cb = nullptr;
            _thread = -1;
        }


    };

private:
    //调度器名称
    std::string _name;
    schedularMutex _mutex;
    
    //线程池
    std::vector<Thread::ThreadPtr> _threads;
    //任务队列
    std::list<ScheduleTask> _tasks;

    std::vector<int> _threadIds;

    // 工作线程数量，不包括use_caller的主线程
    size_t _threadCount = 0;
    // 活跃线程数
    std::atomic<size_t> _activeThreadCount = {0};

    std::atomic<size_t> _idleThreadCount = {0};

    // 是否use caller
    bool _useCaller;
    // use_caller 为true时， 调度器所在线程的调度协程
    Fiber::FiberPtr _rootFiber;    
    // use_caller 为true时， 调度器所在线程的id
    int _rootThread = 0;

    //是否正在停止
    bool _stopping = false;
};

    
} // namespace MySalay




#endif