/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-17 01:04:43
 * @LastEditors: LOSER
 * @LastEditTime: 2024-02-19 00:19:17
 */

#ifndef __FIBER_HPP__
#define __FIBER_HPP__

#include <memory>
#include <functional>

#include <ucontext.h>

#include "other.hpp"
#include "macro.hpp"
#include "thread.hpp"
#include "config.hpp"


namespace MySylar {
    
class Fiber : public std::enable_shared_from_this<Fiber> {

public:
    using FiberPtr = std::shared_ptr<Fiber>;

public:
    /// @brief 协程状态 三类 READY RUNNING TERM
    enum State {
        READY,//就绪态，刚创建或者yield之后的状态
        RUNNING,//运行态，resume之后的状态
        TERM//结束态，协程的回调函数执行完之后为TERM
    };
private:
    /**
     * @description: 无参构造函数，创建线程的第一个协程。也就是线程主函数对应的协程
     * 仅GetThis()方法构造调用 所以定义为私有
     */    
    Fiber();

public:
    /**
     * @brief 构造函数，用于创建用户协程
     * @param[in] cb 协程入口函数
     * @param[in] stacksize 栈大小
     * @param[in] run_in_scheduler 本协程是否参与调度器调度，默认为true
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool runInScheduler = true);

    ~Fiber();
    
    /**
     * @brief 重置协程状态和入口函数，复用栈空间，不重新创建栈
     * @param[in] cb 
     */

    void Reset(std::function<void()> cb);
    
    /**
     * @brief 将当前协程切到到执行状态
     * @details 当前协程和正在运行的协程进行交换，前者状态变为RUNNING，后者状态变为READY
     */
    
    void Resume();

    /**
     * @brief 当前协程让出执行权
     * @details 当前协程与上次resume时退到后台的协程进行交换，前者状态变为READY，后者状态变为RUNNING
     */

    void Yield();

    uint64_t GetId() const {return _id;}

    State GetState() const {return _state;}

public:
    static void SetThis(Fiber * f);

    static Fiber::FiberPtr GetThis();

    static uint64_t TotalFibers();

    static void MainFunc();

    static uint64_t GetFiberId();

private:
    uint64_t _id = 0;
    
    uint32_t _stacksize = 0;

    State _state = READY;
    
    ucontext_t _ctx;

    void * _stackAddr;

    std::function<void()> _cb;

    bool _runInScheduler;
};

}

#endif