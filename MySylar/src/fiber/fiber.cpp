/*
 * @Description:
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-17 01:04:35
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-15 02:53:53
 */

#include <atomic>

#include "fiber.hpp"
#include "scheduler.hpp"

namespace MySylar {

static Logger::loggerPtr fiberLogger = LOG_NAME("system");

//当前线程正在运行的协程
static thread_local Fiber * _curFiber = nullptr;
/// 线程局部变量，当前线程的主协程，切换到这个协程，就相当于切换到了主线程中运行，智能指针形式
static thread_local Fiber::FiberPtr _threadFiber = nullptr;

/// 全局静态变量，用于生成协程id
static std::atomic<uint64_t> _fiberId{0};
/// 全局静态变量，用于统计当前的协程数
static std::atomic<uint64_t> _fiberCount{0};

//协程栈大小，可通过配置文件获取，默认128k
static ConfigVar<uint32_t>::configVarPtr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

/**
 * @brief malloc栈内存分配器
 */
class MallocStackAllocator {
public:
    static void *Alloc(size_t size) { return malloc(size); }
    static void Dealloc(void *vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

Fiber::Fiber()
{
    SetThis(this);
    _state = RUNNING;

    //获取当前进程的上下文并保存在_ctx变量中
    if (getcontext(&_ctx)) {
        MYSYLAR_ASSERT2(false, "getcontext");
    }
    
    ++_fiberCount;
    _id = _fiberId++;

    DEBUG_LOG(fiberLogger) << "Fiber::Fiber() main id = " << _id;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool runInScheduler)
    : _id(_fiberId++)
    , _cb(cb)
    , _runInScheduler(runInScheduler) {
    ++_fiberCount;
    
    _stacksize = stacksize ? stacksize : g_fiber_stack_size->GetValue();
    _stackAddr     = StackAllocator::Alloc(_stacksize);

    //获取当前进程的上下文并保存在_ctx变量中
    if (getcontext(&_ctx)) {
        MYSYLAR_ASSERT2(false, "getcontext");
    }

    _ctx.uc_link          = nullptr;
    _ctx.uc_stack.ss_sp   = _stackAddr;
    _ctx.uc_stack.ss_size = _stacksize;

    // 修改由getcontext获取到的上下文指针ucp，将其与一个函数func进行绑定，支持指定func运行时的参数，
    // 在调用makecontext之前，必须手动给ucp分配一段内存空间，存储在ucp->uc_stack中，这段内存空间将作为func函数运行时的栈空间，
    // 同时也可以指定ucp->uc_link，表示函数运行结束后恢复uc_link指向的上下文，
    // 如果不赋值uc_link，那func函数结束时必须调用setcontext或swapcontext以重新指定一个有效的上下文，否则程序就跑飞了
    // makecontext执行完后，ucp就与函数func绑定了，调用setcontext或swapcontext激活ucp时，func就会被运行
    makecontext(&_ctx, &Fiber::MainFunc, 0);

    DEBUG_LOG(fiberLogger) << "Fiber::Fiber() id = " << _id;
}

Fiber::~Fiber()
{   
    DEBUG_LOG(fiberLogger) << "Fiber::~Fiber() id = " << _id;
    --_fiberCount;
    if (_stackAddr) {
        // 有栈，说明是子协程，需要确保子协程一定是结束状态
        MYSYLAR_ASSERT(_state == TERM);
        StackAllocator::Dealloc(_stackAddr, _stacksize);
        DEBUG_LOG(fiberLogger) << "dealloc stack, id = " << _id;
    } else {
        // 没有栈，说明是线程的主协程
        MYSYLAR_ASSERT(!_cb);              // 主协程没有cb
        MYSYLAR_ASSERT(_state == RUNNING); // 主协程一定是执行状态

        Fiber *cur = _curFiber; // 当前协程就是自己
        if (cur == this) {
            SetThis(nullptr);
        }
    }

}

/**
 * 这里为了简化状态管理，强制只有TERM状态的协程才可以重置，但其实刚创建好但没执行过的协程也应该允许重置的
 */
void Fiber::Reset(std::function<void()> cb) {
    MYSYLAR_ASSERT(_stackAddr);
    MYSYLAR_ASSERT(_state == TERM);
    _cb = cb;
    if (getcontext(&_ctx)) {
        MYSYLAR_ASSERT2(false, "getcontext");
    }

    _ctx.uc_link          = nullptr;
    _ctx.uc_stack.ss_sp   = _stackAddr;
    _ctx.uc_stack.ss_size = _stacksize;

    makecontext(&_ctx, &Fiber::MainFunc, 0);
    _state = READY;
}

void Fiber::Resume() {
    MYSYLAR_ASSERT(_state != TERM && _state != RUNNING);
    SetThis(this);
    _state = RUNNING;

    //如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
    if (_runInScheduler) {
        if (swapcontext(&(Scheduler::GetMainFiber()->_ctx), &_ctx)) {
            MYSYLAR_ASSERT2(false, "swapcontext");
        }
    } else {
        if (swapcontext(&(_threadFiber->_ctx), &_ctx)) {
            MYSYLAR_ASSERT2(false, "swapcontext");
        }
    }
}

void Fiber::Yield() {
    /// 协程运行完之后会自动yield一次，用于回到主协程，此时状态已为结束状态
    MYSYLAR_ASSERT(_state == RUNNING || _state == TERM);
    SetThis(_threadFiber.get());
    if (_state != TERM) {
        _state = READY;
    }

    // 恢复ucp指向的上下文，同时将当前的上下文存储到oucp中，
    // 和setcontext一样，swapcontext也不会返回，而是会跳转到ucp上下文对应的函数中执行，相当于调用了函数
    // swapcontext是sylar非对称协程实现的关键，线程主协程和子协程用这个接口进行上下文切换
    
    // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
    if (_runInScheduler) {
        if (swapcontext(&_ctx, &(Scheduler::GetMainFiber()->_ctx))) {
            MYSYLAR_ASSERT2(false, "swapcontext");
        }
    } else {
        if (swapcontext(&_ctx, &(_threadFiber->_ctx))) {
            MYSYLAR_ASSERT2(false, "swapcontext");
        }
    }
}

void Fiber::SetThis(Fiber *f)
{
    _curFiber = f;
}

Fiber::FiberPtr Fiber::GetThis()
{
    if(_curFiber) {
        return _curFiber->shared_from_this();
    }

    Fiber::FiberPtr mainFiber(new Fiber);
    MYSYLAR_ASSERT(_curFiber == mainFiber.get());
    _threadFiber = mainFiber;
    return _curFiber->shared_from_this();

}

uint64_t Fiber::TotalFibers()
{
    return 0;
}

void Fiber::MainFunc()
{
    Fiber::FiberPtr curFiber = GetThis();
    MYSYLAR_ASSERT(curFiber);

    curFiber->_cb();
    curFiber->_cb = nullptr;
    curFiber->_state = TERM;

    auto rawPtr = curFiber.get();
    curFiber.reset();
    rawPtr->Yield();
}

uint64_t Fiber::GetFiberId()
{
    if(_curFiber) {
        return _curFiber->GetId();
    }
    return 0;
}


}

