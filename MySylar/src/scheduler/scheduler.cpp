/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-28 21:58:39
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-15 05:29:11
 */
#include "scheduler.hpp"

namespace MySylar
{

static MySylar::Logger::loggerPtr _logger = LOG_NAME("system");

// 当前线程的调度器， 同一个调度器下的所有线程共享同一个实例
static thread_local Scheduler * _scheduler = nullptr;
// 当前线程的调度协程， 每个线程都独有一份
static thread_local Fiber * _scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool useCaller, const std::string &name) {
    MYSYLAR_ASSERT(threads > 0);
    
    _useCaller = useCaller;
    _name = name;

    if (useCaller) {
        --threads;
        Fiber::GetThis();//初始化
        MYSYLAR_ASSERT(GetThis() == nullptr);
        _scheduler = this;


       /**
         * caller线程的主协程不会被线程的调度协程run进行调度，而且，线程的调度协程停止时，应该返回caller线程的主协程
         * 在user caller情况下，把caller线程的主协程暂时保存起来，等调度协程结束时，再resume caller协程
         */ DEBUG_LOG(_logger) << "Schedular() useCaller";

        _rootFiber.reset(new Fiber(std::bind(&Scheduler::Run, this), 0, false));

        Thread::SetThreadName(name);
        _scheduler_fiber = _rootFiber.get();
        _rootThread = MySylar::GetThreadId();
        _threadIds.push_back(_rootThread);
    } else {
        _rootThread = -1;
    }
    
    _threadCount = threads;
}
Scheduler::~Scheduler()
{
    DEBUG_LOG(_logger) << "~Schedular()";
    MYSYLAR_ASSERT(_stopping);
    if (GetThis() == this) {
        _scheduler = nullptr;
    }
}
Scheduler *Scheduler::GetThis()
{
    return _scheduler;
}
Fiber *Scheduler::GetMainFiber()
{
    return _scheduler_fiber;
}
void Scheduler::Start()
{
    DEBUG_LOG(_logger) << "start()";
    schedularMutex::lock lock(_mutex);
    
    if(_stopping) {
        ERROR_LOG(_logger) << "Schedular is stopped";
        return;
    }
    
    MYSYLAR_ASSERT(_threads.empty());
    _threads.resize(_threadCount);
    //只使用main函数线程进行调度，那start相当于什么也没做 _threadCount == 0
    for(size_t i = 0; i < _threadCount; i++) {
        _threads[i].reset(new Thread(std::bind(&Scheduler::Run, this), _name + "_" + std::to_string(i)));
        _threadIds.push_back(_threads[i]->GetId());
    }
}
void Scheduler::Stop()
{
    DEBUG_LOG(_logger) << "stop";

    if(Stopping()) {
        return;
    }

    _stopping = true;

    /// 如果use caller，那只能由caller线程发起stop
    if (_useCaller) {
        MYSYLAR_ASSERT(GetThis() == this);
    } else {
        MYSYLAR_ASSERT(GetThis() != this);
    }

    for(size_t i = 0; i < _threadCount; i++) {
        Tickle();
    }

    if(_rootFiber) {
        Tickle();
    }

    if(_rootFiber) {
        DEBUG_LOG(_logger) << "_rootFiber end1";
        _rootFiber->Resume();
        DEBUG_LOG(_logger) << "_rootFiber end2";
    }

    std::vector<Thread::ThreadPtr> threads;
    {
        schedularMutex::lock lock(_mutex);
        threads.swap(_threads);
    }

    for( auto & i : threads) {
        i->Join();
    }
}
void Scheduler::Tickle()
{
    DEBUG_LOG(_logger) << "Tickle()";
}
void Scheduler::Run()
{
    DEBUG_LOG(_logger) << "run";
    //setHookEnable(true);
    SetThis();

    if(MySylar::GetThreadId() != _rootThread) {
        _scheduler_fiber = Fiber::GetThis().get();
    }
    //这里this 由IO调度的话 是iomanager的this
    Fiber::FiberPtr idleFiber(new Fiber(std::bind(&Scheduler::Idle, this)));
    Fiber::FiberPtr cbFiber;

    ScheduleTask task;
    while(true) {
        task.reset();
        bool tickleMe = false;//是否tickle其他线程进行任务调度
        {
        schedularMutex::lock lock(_mutex);
        auto it = _tasks.begin();
        while(it != _tasks.end()) {
            if (it->_thread != -1 && it->_thread != GetThreadId()) {
                // 指定了调度线程，但不是在当前线程上调度，标记一下需要通知其他线程进行调度，然后跳过这个任务，继续下一个
                ++it;
                tickleMe = true;
                continue;
            }

             // 找到一个未指定线程，或是指定了当前线程的任务
            MYSYLAR_ASSERT(it->_fiber || it->_cb);

            // [BUG FIX]: hook IO相关的系统调用时，在检测到IO未就绪的情况下，会先添加对应的读写事件，再yield当前协程，等IO就绪后再resume当前协程
            // 多线程高并发情境下，有可能发生刚添加事件就被触发的情况，如果此时当前协程还未来得及yield，则这里就有可能出现协程状态仍为RUNNING的情况
            // 这里简单地跳过这种情况，以损失一点性能为代价，否则整个协程框架都要大改
            if(it->_fiber && it->_fiber->GetState() == Fiber::RUNNING) {
                ++it;
                continue;
            }

            // 当前调度线程找到一个任务，准备开始调度，将其从任务队列中剔除，活动线程数加1
            task = *it;
            _tasks.erase(it++);
            ++_activeThreadCount;
            break;
        }
            // 当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
            tickleMe |= (it != _tasks.end());
        }
        
        if (tickleMe) {
            Tickle();
        }

        if (task._fiber) {
            // resume协程，resume返回时，协程要么执行完了，要么半路yield了，总之这个任务就算完成了，活跃线程数减一
            task._fiber->Resume();
            --_activeThreadCount;
            task.reset();
        } else if (task._cb) {
            if (cbFiber) {
                cbFiber->Reset(task._cb);
            } else {
                cbFiber.reset(new Fiber(task._cb));
            }
            task.reset();
            cbFiber->Resume();
            --_activeThreadCount;
            cbFiber.reset();
        } else {
            // 进到这个分支情况一定是任务队列空了，调度idle协程即可
            DEBUG_LOG(_logger) << "task empty() after idleFiber->Resume()";
            if (idleFiber->GetState() == Fiber::TERM) {
                // 如果调度器没有调度任务，那么idle协程会不停地resume/yield，不会结束，如果idle协程结束了，那一定是调度器停止了
                DEBUG_LOG(_logger) << "idle fiber term";
                break;
            }
            ++_idleThreadCount;
            idleFiber->Resume();
            --_idleThreadCount;
        }
    }

    DEBUG_LOG(_logger) << "Schedular::run() exit";
}
void Scheduler::Idle()
{
    DEBUG_LOG(_logger) << "Schedular::Idle()";
    while (!Stopping()) {
        Fiber::GetThis()->Yield();
    }
    
}

bool Scheduler::Stopping()
{
    schedularMutex::lock lock(_mutex);
    return _stopping && _tasks.empty() && _activeThreadCount == 0;
}

void Scheduler::SetThis()
{
    _scheduler = this;
}

} // namespace MySylar
