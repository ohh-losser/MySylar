/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-03-03 18:27:05
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-15 05:31:17
 */

#include "IOManager.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

MySylar::Logger::loggerPtr _logger = LOG_ROOT();

int sockfd;

void watch_io_read();

void do_io_write() {
    INFO_LOG(_logger) << "write callback";
    int so_err;
    socklen_t len = size_t(so_err);
    //获取套接字的各种状态信息，包括错误代码。
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_err, &len);
    if(so_err) {
        INFO_LOG(_logger) << "connect fail";
        return;
    }

    INFO_LOG(_logger) << "connect success";
}

void do_io_read() {
    INFO_LOG(_logger) << "read callback";
    char buf[1024] = {0};
    int readlen = 0;
    readlen = read(sockfd, buf, sizeof(buf));
    if(readlen > 0) {
        buf[readlen] = '\0';
        INFO_LOG(_logger) << "read " << readlen << " bytes, read: " << buf;
    } else if (readlen == 0) {
        INFO_LOG(_logger) << "peer closed";
        close(sockfd);
        return;
    } else {
        ERROR_LOG(_logger) << "err, errno=" << errno << ", errstr=" << strerror(errno);
        close(sockfd);
        return;
    }
    // read之后重新添加读事件回调，这里不能直接调用addEvent，因为在当前位置fd的读事件上下文还是有效的，直接调用addEvent相当于重复添加相同事件
    MySylar::IOManager::GetThis()->Schedule(watch_io_read);
}

void watch_io_read() {
    INFO_LOG(_logger) << "watch_io_read";
    MySylar::IOManager::GetThis()->AddEvent(sockfd, MySylar::IOManager::READ, do_io_read);
}


void test_io() {
    //socket 进程间双向通信抽象的端点
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    MYSYLAR_ASSERT(sockfd > 0);
    //非阻塞io
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    //封装端口号和IP等信息的结构体
    sockaddr_in servadder;
    memset(&servadder, 0, sizeof(servadder));
    servadder.sin_family = AF_INET;
    //转换成大端字节序
    servadder.sin_port = htons(1234);
    //将点分十进制字符串，转换为网络字节序整数
    inet_pton(AF_INET, "192.168.106.1", &servadder.sin_addr.s_addr);
    //建立连接
    int retval = connect(sockfd, (const sockaddr*)&servadder, sizeof(servadder));
    if(retval != 0) {
        if(errno == EINPROGRESS) {
            INFO_LOG(_logger) << "EINPROGRESS";

            MySylar::IOManager::GetThis()->AddEvent(sockfd, MySylar::IOManager::WRITE, do_io_write);
            MySylar::IOManager::GetThis()->AddEvent(sockfd, MySylar::IOManager::READ, do_io_read);
        
        } else {
            ERROR_LOG(_logger) << "connect error, errno:" << errno << ", errstr:" << strerror(errno);
        }
    } else {
        ERROR_LOG(_logger) << "else, errno:" << errno << ", errstr:" << strerror(errno);
    }
    DEBUG_LOG(_logger) << "test_io end";
}

void test_iomanager() {
    MySylar::IOManager iom;
    auto logmgr = MySylar::singleLoggerManager::GetInstance();
    auto syslogger = logmgr->GetLogger("system");
    syslogger->SetLevel(MySylar::LogLevel::DEBUG);
    MySylar::FileLogAppender::logAppenderPtr logFile(new MySylar::FileLogAppender("./log2.txt"));
    syslogger->AddAppender(logFile);
    iom.Schedule(test_io);
}

int main(int argc, char * argv[]) {
    MySylar::singelEnv::GetInstance()->Init(argc, argv);
    MySylar::Config::LoadFromConfDir(MySylar::singelEnv::GetInstance()->GetConfigPath());

    MySylar::FileLogAppender::logAppenderPtr logFile(new MySylar::FileLogAppender("./log.txt"));
    _logger->AddAppender(logFile);
    _logger->SetLevel(MySylar::LogLevel::DEBUG);
    DEBUG_LOG(_logger) << "debug";

    test_iomanager();

    return 0;
}