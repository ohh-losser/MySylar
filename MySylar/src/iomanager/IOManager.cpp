/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-03-02 19:57:47
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-15 05:51:54
 */
#include "IOManager.hpp"
#include "log.hpp"

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>



namespace MySylar{

static Logger::loggerPtr _logger = LOG_NAME("system");

MySylar::IOManager::IOManager(size_t threads, bool use_caller, const std::string &name) 
:Scheduler(threads, use_caller, name){
    // 创建epoll实例
    _epfd = epoll_create(5000);
    MYSYLAR_ASSERT(_epfd > 0);
    // 创建pipe，获取_tickleFds[2] 其中m_tickleFds[0]是管道的读端，m_tickleFds[1]是管道的写端
    int rtval = pipe(_tickleFds);
    MYSYLAR_ASSERT(!rtval);

    //关注pipe读句柄的可读事件, 用于tickle(拍一拍)协程调度， 通过data.fd 保存描述符
    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = _tickleFds[0];

    // 非阻塞方式，配合边缘触发
    rtval = fcntl(_tickleFds[0], F_SETFL, O_NONBLOCK);
    MYSYLAR_ASSERT(!rtval);
    
    // 将管道的读描述符加入epoll多路复用， 如果管道可读， idle中的epoll_wait会返回
    rtval = epoll_ctl(_epfd, EPOLL_CTL_ADD, _tickleFds[0], &event);
    MYSYLAR_ASSERT(!rtval);

    ContextResize(32);

    //开启Scheduler, 也就是说IOManager创建即可调度协程
    Start();
}

IOManager::~IOManager()
{
    DEBUG_LOG(_logger) << "~IOManager"; 
    Stop();
    close(_epfd);
    close(_tickleFds[0]);
    close(_tickleFds[1]);

    for(size_t i = 0; i < _fdContexts.size(); ++i) {
        if(_fdContexts[i]) {
            delete _fdContexts[i];
        }
    }
}

int IOManager::AddEvent(int fd, Event event, std::function<void()> cb)
{
    FdContext * fd_ctx = nullptr;
    RWMutexType::ReadLock lock(_mutex);
    //找到fd对应的FdContext, 如果不存在，那就分配一个
    if ((int)_fdContexts.size() > fd) {//存在
        fd_ctx = _fdContexts[fd];
        lock.Unlock();
    } else { //不存在
        lock.Unlock();
        RWMutexType::WriteLock lock2(_mutex);
        ContextResize(fd * 1.5);
        fd_ctx = _fdContexts[fd];
    }

    //同一个fd不允许重复添加相同的事件
    FdContext::MutexType::lock lock2(fd_ctx->mutex);
    if(MYSYLAR_UNLIKELY(fd_ctx->events & event)) {
        ERROR_LOG(_logger) << "AddEvent assert fd=" << fd
                           << " event=" << (EPOLL_EVENTS)event
                           << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;

        MYSYLAR_ASSERT(!(fd_ctx->events & event));
    }

    //将新的事件加入epoll_wait, 使用epoll_event的私有指针存储FdContext的位置
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int retval = epoll_ctl(_epfd, op, fd, &epevent);
    if (retval) {
        ERROR_LOG(_logger) << "epoll_ctl(" << _epfd << ","
                           << "op" << "," << fd << "," << (EPOLL_EVENTS)epevent.events << ")"
                           << "retval" << "(" << errno << ")" << strerror(errno) << ") fd_ctx->events="
                           << (EPOLL_EVENTS)fd_ctx->events;

        return -1;
    }
    
    //待执行IO事件数加1
    ++_pendingEventCount;

    // 找到这个fd的event事件对应的EventContext, 对其中的scheduler, cb, fiber 进行赋值
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext & event_ctx = fd_ctx->GetEventContext(event);
    MYSYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    //赋值scheduler和回调函数，如果回调函数为空，则把当前协程当成回调执行体
    event_ctx.scheduler = Scheduler::GetThis();
    if(cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        MYSYLAR_ASSERT2(event_ctx.fiber->GetState() == Fiber::RUNNING, "state=" << event_ctx.fiber->GetState());
    }

    return 0;
}

bool IOManager::DelEvent(int fd, Event event)
{
    //找到fd对应的FdContext
    RWMutexType::ReadLock lock(_mutex);
    if((int)_fdContexts.size() <= fd) {
        return false;
    }

    FdContext * fd_ctx = _fdContexts[fd];
    lock.Unlock();

    FdContext::MutexType::lock lock2(fd_ctx->mutex);
    if(MYSYLAR_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    // 清除指定的事件， 表示不关心这个事件了， 如果清除之后结果为0， 则从epoll_wait中删除该文件描述符
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int retval = epoll_ctl(_epfd, op, fd, &epevent);
    if(retval) {
        ERROR_LOG(_logger) << "epoll_ctl(" << _epfd << ","
                           << "op" << "," << fd << "," << (EPOLL_EVENTS)epevent.events << ")"
                           << "retval" << "(" << errno << ")" << strerror(errno) << ") fd_ctx->events="
                           << (EPOLL_EVENTS)fd_ctx->events;
    }

    // 待执行事件数减1
    --_pendingEventCount;
    // 重置该fd对应的event事件上下文
    fd_ctx->events = new_events;
    FdContext::EventContext & event_ctx = fd_ctx->GetEventContext(event);
    fd_ctx->ResetEventContext(event_ctx);

    return true;
}

bool IOManager::CancelEvent(int fd, Event event)
{
    //找到fd对应的FdContext
    RWMutexType::ReadLock lock(_mutex);
    if((int)_fdContexts.size() <= fd) {
        return false;
    }

    FdContext * fd_ctx = _fdContexts[fd];
    lock.Unlock();

    FdContext::MutexType::lock lock2(fd_ctx->mutex);
    if(MYSYLAR_UNLIKELY(!(fd_ctx->events & event))) {
        return false;
    }

    // 删除事件
    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int retval = epoll_ctl(_epfd, op, fd, &epevent);
    if(retval) {
        ERROR_LOG(_logger) << "epoll_ctl(" << _epfd << ","
                           << "op" << "," << fd << "," << (EPOLL_EVENTS)epevent.events << ")"
                           << "retval" << "(" << errno << ")" << strerror(errno) << ") fd_ctx->events="
                           << (EPOLL_EVENTS)fd_ctx->events;
    }

    //取消之前触发一次事件
    fd_ctx->triggerEvent(event);

    // 待执行事件数减1
    --_pendingEventCount;

    return true;
}

bool IOManager::CancelAllEvent(int fd)
{
        //找到fd对应的FdContext
    RWMutexType::ReadLock lock(_mutex);
    if((int)_fdContexts.size() <= fd) {
        return false;
    }

    FdContext * fd_ctx = _fdContexts[fd];
    lock.Unlock();

    FdContext::MutexType::lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int retval = epoll_ctl(_epfd, op, fd, &epevent);
    if(retval) {
        ERROR_LOG(_logger) << "epoll_ctl(" << _epfd << ","
                           << "op" << "," << fd << "," << (EPOLL_EVENTS)epevent.events << ")"
                           << "retval" << "(" << errno << ")" << strerror(errno) << ") fd_ctx->events="
                           << (EPOLL_EVENTS)fd_ctx->events;
    }

    //触发全部已经注册的事件
    if(fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --_pendingEventCount;
    }
    if(fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --_pendingEventCount;
    }

    MYSYLAR_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager *IOManager::GetThis()
{
    return dynamic_cast<IOManager *>(Scheduler::GetThis());
}

void MySylar::IOManager::Tickle()
{
    DEBUG_LOG(_logger) << "IOManager Tickle()";
    if(!HasIdleThreads()) {
        return;
    }

    int retval = write(_tickleFds[1], "T", 1);
    MYSYLAR_ASSERT(retval == 1);
}

void IOManager::Idle()
{
    DEBUG_LOG(_logger) << "IOManager Idle()";

    const uint64_t MAX_EVENTS = 128;
    epoll_event * events = new epoll_event[MAX_EVENTS]();
    // 对于申请的动态数组来说，shared_ptr 指针默认的释放规则是不支持释放数组的，只能自定义对应的释放规则
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event * ptr) {
        delete [] ptr;
    });

    while(true) {
        if(Stopping()) {
            DEBUG_LOG(_logger) << "name=" << GetName() << " idle stopping exit";
            break;
        }

        static const int MAX_TIMEOUT = 5000;
        // int retval = 0;
        // do {
        //     retval = epoll_wait(_epfd, events, MAX_EVENTS, MAX_TIMEOUT);
        //     if(retval < 0) {
        //         if (errno == EINTR) {
        //             continue;
        //         }
        //         ERROR_LOG(_logger) << "epoll_wait() epfd retval = " << errno << " errmsg = " << strerror(errno);
        //         break;
        //     }

        // }while(true);

        //定时器再改回去

        int retval = epoll_wait(_epfd, events, MAX_EVENTS, MAX_TIMEOUT);
        if(retval < 0) {
            if(errno == EINTR) {
                continue;
            }
            ERROR_LOG(_logger) << "epoll_wait() epfd retval = " << errno << " errmsg = " << strerror(errno);
            break;
        }

        DEBUG_LOG(_logger) << "epoll_wait retval = " << retval;

        // 遍历所有发生的事件，根据epoll_event的私有指针找到对应的FdContext，进行事件处理
        for(int idx = 0; idx < retval; ++idx) {
            epoll_event & event = events[idx];
            if(event.data.fd == _tickleFds[0]) {
                uint8_t dummy[256];
                // ticklefd[0] 用于通知协程调度， 这时只需要把管道里的内容读完即可, 本轮idle结束 Scheduler::run 会重新执行调度
                while (read(_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext * fd_ctx = (FdContext*) event.data.ptr;
            FdContext::MutexType::lock lock(fd_ctx->mutex);
            
            /**
             * EPOLLERR: 出错，比如写读端已经关闭的pipe
             * EPOLLHUP: 套接字对端关闭
             * 出现这两种事件，应该同时触发fd的读和写事件，否则有可能出现注册的事件永远执行不到的情况
             */

            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
            }

            int real_event = NONE;

            if(event.events & EPOLLIN) {
                real_event |= READ;
            }

            if(event.events & EPOLLOUT) {
                real_event |= WRITE;
            }

            if((fd_ctx->events & real_event) == NONE) {
                continue;
            } 

            //剔除已经发生的事件，将剩下的事件重新加入epoll_wait，
            //如果剩下的事件为0， 表示这个fd以及不需要关注了 直接从epoll中删除
            int left_events = (fd_ctx->events & ~real_event) ;
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int retval2 = epoll_ctl(_epfd, op, fd_ctx->fd, &event);
            if(retval2) {
                ERROR_LOG(_logger) << "epoll_ctl() epfd retval = " << errno << " errmsg = " << strerror(errno);
                continue;
            }
            
            //处理已经发生的事件，，也就是让调度器调度指定的函数或者协程
            if(real_event & READ) {
                fd_ctx->triggerEvent(READ);
                --_pendingEventCount;
            }

            if(real_event & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --_pendingEventCount;
            }
        } // end for
        
        /**
         * 一旦处理完所有的事件，idle协程yield，这样可以让调度协程(Scheduler::run)重新检查是否有新任务要调度
         * 上面triggerEvent实际也只是把对应的fiber重新加入调度，要执行的话还要等idle协程退出
         */
        Fiber::FiberPtr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->Yield();
    }
}
void IOManager::ContextResize(size_t size)
{
    _fdContexts.resize(size);

        for (size_t i = 0; i < _fdContexts.size(); ++i) {
        if (!_fdContexts[i]) { // 不存在就新添加
            _fdContexts[i] = new FdContext;
            _fdContexts[i]->fd = i;
        }
    }
}
bool IOManager::Stopping()
{
    // 对于IOManager而言，必须等所有待调度的IO事件都执行完了才可以退出
    return _pendingEventCount == 0 && Scheduler::Stopping();
}

void IOManager::FdContext::triggerEvent(Event event)
{    
    // 待触发的事件必须已被注册过
    MYSYLAR_ASSERT(events & event);
    /**
     * 清除该事件，表示不再关注该事件了
     * 也就是说，注册的IO事件是一次性的，如果想持续关注某个socket fd的读写事件，那么每次触发事件之后都要重新添加
     */
    events = (Event)(events & ~event);
    // 调度对应的协程
    EventContext &ctx = GetEventContext(event);
    if (ctx.cb) {
        ctx.scheduler->Schedule(ctx.cb);
    } else {
        ctx.scheduler->Schedule(ctx.fiber);
    }
    ResetEventContext(ctx);
    return;

}

}
