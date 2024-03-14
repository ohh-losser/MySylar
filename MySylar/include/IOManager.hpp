#ifndef __IOMANAGER__HPP__
#define __IOMANAGER__HPP__

#include "scheduler.hpp"

namespace MySylar{

class IOManager : public Scheduler {
public:
    using IOManagerPtr = std::shared_ptr<IOManager>;
    using RWMutexType = RWMutex;

/// @brief 继承epoll对io事件的定义
    enum Event {
        NONE = 0x0,
        READ = 0x1, //读事件 EPOLLIN
        WRITE = 0x4, //写事件 EPOLLOUT
    };

    /// @brief socket fd 上下文类
    struct FdContext {
        using MutexType = Mutex;
        
        /**
         * @description: 事件上下文类
         * @details fd的每个事件都有一个事件的上下文，保存这个事件的回调函数以及执行回调函数的调度器
         *          简化成只有读事件和写事件
         */    
        struct EventContext {
            //执行事件回调的调度器
            Scheduler * scheduler = nullptr;
            // 事件回调协程
            Fiber::FiberPtr fiber;
            // 事件回调函数
            std::function<void()> cb;

        };

        /**
         * @description: 获取事件上下文类
         * @param {Event} event 事件类
         * @return 返回对应事件的上下文类
         */    
        EventContext & GetEventContext(Event event) {
            switch (event) {
            case IOManager::READ:
                return read;
            case IOManager::WRITE:
                return write;
            default:
                MYSYLAR_ASSERT2(false, "getContext");
            }
            throw std::invalid_argument("getContext invalid event");
        }

        void ResetEventContext(EventContext & ctx) {
            ctx.scheduler = nullptr;
            ctx.fiber.reset();
            ctx.cb = nullptr;
        }

        /**
         * @description: 触发事件
         * @param {Event} event 事件类
         @details 根据事件类型调用对应上下文结构中的调度器去调度回调协程或回调函数
        */
        void triggerEvent(Event event);

        // 读事件上下文
        EventContext read;
        // 写事件上下文
        EventContext write;
        // 事件光联的句柄
        int fd = 0;
        // fd 关心那些事件
        Event events = NONE;
        
        MutexType mutex;
    };
public:
    /**
     * @description: 构造函数
     * @param {size_t} threads 线程数量
     * @param {bool} use_caller 是否将调用线程包含进去
     * @param {string &} name 调度器的名称
     * @return {*}
     */
    IOManager(size_t threads = 1, bool use_caller = true, const std::string & name = "IOManager");

    ~IOManager();

    int AddEvent(int fd, Event event, std::function<void()> cb = nullptr);

    //删除事件 不会触发事件
    bool DelEvent(int fd, Event event);
    
    //取消事件 取消前会触发一次回调事件
    bool CancelEvent(int fd, Event event);
    
    //取消所有事件，同上
    bool CancelAllEvent(int fd);

    static IOManager * GetThis();

protected:
    /**
     * @description: 通知调度器有任务要调度
     * @details: 写pipe让idle协程从epoll_wait退出 待idle协程yield之后Scheduler::run 就可以调度其他任务
     *           如果当前没有空闲调度线程，那就没必要发通知
     */    
    void Tickle() override;
    /**
     * @brief: idle协程
     * @details: 对于IO协程调度来说，应阻塞在等待IO事件上， idle退出的时机是epoll_wait返回， 
     *  对应的操作是tickle或注册IO事件就绪。
     *  调度器无调度任务时会阻塞idle协程上.
     *  对IO调度器而言，idle状态应该关注两件事：
     *      1、有没有新的任务对应Scheduler::scheduler()，如果有新的调度任务，那应该立即退出idle状态，并执行对应的任务
     *      2、关注当前注册大的所有IO事件有没有触发，如果有触发，那么应该执行IO事件的回调函数
     */
    void Idle() override;

    void ContextResize(size_t size);

    bool Stopping() override;

private:
    // epoll 文件句柄
    int _epfd = 0;
    // pipe 文件句柄， fd[0] 读端 fd[1] 写端
    int _tickleFds[2];
    // 当前等级执行的IO事件数量
    std::atomic<size_t> _pendingEventCount = {0};
    // IOManager的mutex
    RWMutexType _mutex;
    // socket事件上下文的容器
    std::vector<FdContext *> _fdContexts;

};

}

#endif



