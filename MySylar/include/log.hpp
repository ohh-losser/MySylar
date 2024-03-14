/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-17 22:36:49
 * @LastEditors: LOSER
 * @LastEditTime: 2024-02-01 17:20:25
 */
#ifndef __SYLAR_LOG_HPP__
#define __SYLAR_LOG_HPP__

#include <map>
#include <memory>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <functional>

#include <stdarg.h>

#include <yaml-cpp/yaml.h>

#include "mutex.hpp"
#include "other.hpp"
#include "Singleton.hpp"

/**
 * @brief 获取root日志器
 */
#define LOG_ROOT() MySylar::singleLoggerManager::GetInstance()->GetRoot()

/**
 * @brief 获取指定名称的日志器
 */
#define LOG_NAME(name) MySylar::singleLoggerManager::GetInstance()->GetLogger(name)

#define LEVEL_LOG(logger, level) \
    if(level <= logger->GetLevel()) \
        MySylar::LogEventWrap(logger, MySylar::LogEvent::logEventPtr(new MySylar::LogEvent(logger->GetName(), \
        level, __FILE__, __LINE__, MySylar::GetElapsedMS() - logger->GetCreateTime(), \
        MySylar::GetThreadId(), MySylar::GetFiberId(), time(0), MySylar::GetThreadName()))).GetLogEvent()->GetSS()

#define INFO_LOG(logger) LEVEL_LOG(logger, MySylar::LogLevel::INFO)

#define ERROR_LOG(logger) LEVEL_LOG(logger, MySylar::LogLevel::ERROR)

#define DEBUG_LOG(logger) LEVEL_LOG(logger, MySylar::LogLevel::DEBUG)

/// @brief 
namespace MySylar {

class LogLevel {
public:
    /**
     * @description:日志级别枚举 
     */
    enum Level {
        
        FATAL = 0,

        ALERT = 100,

        CRIT = 200,

        ERROR = 300,

        WARN = 400,

        NOTICE = 500,

        INFO = 600,

        DEBUG = 700,

        NOTSET = 800,
    };

    /**
     * @description:日志级别转字符串 
     * @param {Level} level
     * @return 字符串形式的日志级别
     * @author: LOSER
     */    
    static const char * ToString(LogLevel::Level level);
    /**
     * @description:str转日志级别 
     * @param {string} &str
     * @return 日志级别
     * @author: LOSER
     */
    static LogLevel::Level FromString(const std::string &str);
};

/**
 * @description:日志事件 
 */
class LogEvent {
public:
    typedef std::shared_ptr<LogEvent> logEventPtr;

public:
    /**
     * @brief 构造函数
     * @param[in] loggerName 日志器名称
     * @param[in] elevel 日志级别
     * @param[in] fileName 文件名
     * @param[in] lineNum 行号
     * @param[in] elapseTime 从日志器创建开始到当前的累计运行毫秒
     * @param[in] theadId 线程id
     * @param[in] fiberId 协程id
     * @param[in] timeT UTC时间
     * @param[in] threadName 线程名称
     */
    LogEvent(const std::string &loggerName, LogLevel::Level elevel, const char *fileName, 
        int32_t lineNum, int64_t elapseTime, uint32_t threadId, uint64_t fiberId, 
        time_t timeT, const std::string &threadName);

    /**
     * @brief 获取日志级别
     */
    LogLevel::Level GetLevel() const { return _eLevel; }

    /**
     * @brief 获取日志内容
     */
    std::string GetContent() const { return _ssContent.str(); }

    /**
     * @brief 获取文件名
     */
    std::string GetFile() const { return _fileName; }

    /**
     * @brief 获取行号
     */
    int32_t GetLine() const { return _lineNum; }

    /**
     * @brief 获取累计运行毫秒数
     */
    int64_t GetElapse() const { return _elapseTime; }

    /**
     * @brief 获取线程id
     */
    uint32_t GetThreadId() const { return _threadId; }

    /**
     * @brief 获取协程id
     */
    uint64_t GetFiberId() const { return _fiberId; }

    /**
     * @brief 返回时间戳
     */
    time_t GetTime() const { return _timeT; }

    /**
     * @brief 获取线程名称
     */
    const std::string &GetThreadName() const { return _threadName; }

    /**
     * @brief 获取内容字节流，用于流式写入日志
     */
    std::stringstream &GetSS() { return _ssContent; }

    /**
     * @brief 获取日志器名称
     */
    const std::string &GetLoggerName() const { return _loggerName; }

    /**
     * @brief C prinf风格写入日志
     */
    void Printf(const char *fmt, ...);

    /**
     * @brief C vprintf风格写入日志
     */
    void Vprintf(const char *fmt, va_list ap);
private:
    //日志级别
    LogLevel::Level _eLevel;
    //日志内容, 使用stringstream存储，便于流式写入日志
    std::stringstream _ssContent;
    //文件名 效率比std::string高
    const char * _fileName = nullptr;
    // 行号
    int32_t _lineNum = 0;
    // 日志器创建开始到当前的耗时
    int64_t _elapseTime = 0;
    // 线程id
    uint32_t _threadId = 0;
    // 协程id
    uint64_t _fiberId = 0;
    // utc时间戳
    time_t _timeT;
    // 线程名称
    std::string _threadName;
    // 日志器名称
    std::string _loggerName;
};

/**
 * @description: 日志格式
 */
class LogFormatter {
public:
    using logFormatterPtr = std::shared_ptr<LogFormatter>;
public:
    /**
     * @brief 构造函数
     * @param[in] pattern 格式模板，参考sylar与log4cpp
     * @details 模板参数说明：
     * - %%m 消息
     * - %%p 日志级别
     * - %%c 日志器名称
     * - %%d 日期时间，后面可跟一对括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这里的格式字符与C语言strftime一致
     * - %%r 该日志器创建后的累计运行毫秒数
     * - %%f 文件名
     * - %%l 行号
     * - %%t 线程id
     * - %%F 协程id
     * - %%N 线程名称
     * - %%% 百分号
     * - %%T 制表符
     * - %%n 换行
     * 
     * 默认格式：%%d{%%Y-%%m-%%d %%H:%%M:%%S}%%T%%t%%T%%N%%T%%F%%T[%%p]%%T[%%c]%%T%%f:%%l%%T%%m%%n
     * 
     * 默认格式描述：年-月-日 时:分:秒 [累计运行毫秒数] \\t 线程id \\t 线程名称 \\t 协程id \\t [日志级别] \\t [日志器名称] \\t 文件名:行号 \\t 日志消息 换行符
     */

    LogFormatter(const std::string &pattern = "%d{%Y-%m-%d %H:%M:%S} [[%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n]");

    void Init();

    bool IsError() const {return _isError;};
 
    /**
     * @description: 对logAppender的log事件进行格式化，返回格式化日志文本
     * @param {logEventPtr} event
     * @return {格式化日志文本}
     */
    std::string Format(LogEvent::logEventPtr event);
    /**
     * @description: 对logAppender的log事件进行格式化，返回格式化日志流
     * @param std::ostream &os
     * @param {logEventPtr} event
     * @return {格式化日志流}
     */
    std::ostream &Format(std::ostream &os, LogEvent::logEventPtr event);

    std::string GetPattern() const {return _pattern;};

public:
    /**
     * @description: 日志内容格式化项，虚基类，用于派生出不同的格式化项
     * @return {*}
     */    
    class FormatItem {
    public:
        using formatItemPtr = std::shared_ptr<FormatItem>;   
    public:
        /**
         * @description:析构函数 
         */
        virtual ~FormatItem() {}
        /**
         * @description: 格式化日志事件
         * @param {ostream} &os
         * @param {logEventPtr} eventPtr
         */
        virtual void Format(std::ostream &os, LogEvent::logEventPtr eventPtr) = 0;
    };
private:
    //日志格式模板
    std::string _pattern;
    //解析后的格式模板数组
    std::vector<FormatItem::formatItemPtr> _formatItems;
    //是否出错
    bool _isError = false;
};
/**
 * @description:日志输出类，用于派生出不同logAppender 主要为控制台和文件 
 */
class LogAppender {
public:
    using logAppenderPtr = std::shared_ptr<LogAppender>;
    using logAppenderSpinlock = Spinlock;

public:
    LogAppender(LogFormatter::logFormatterPtr defaultFormatter);

    virtual ~LogAppender() {}

    void SetFormatter(LogFormatter::logFormatterPtr formatter);

    LogFormatter::logFormatterPtr GetFormatter();
    /**
     * @description: 写入日志
     * @param {logEventPtr} event
     * @return {*}
     */
    virtual void Log(LogEvent::logEventPtr event) = 0;
    /**
     * @description: 将日志输出目标的配置转成yaml string yaml json的超集
     * @return {*}
     */
    virtual std::string ToYamlString() = 0;

protected:
    //自旋锁 
    logAppenderSpinlock _appenderSpinlock;
    //日志格式器
    LogFormatter::logFormatterPtr _formatter;

    LogFormatter::logFormatterPtr _defaultFormatter;
};

class StdoutLogAppender : public LogAppender {
public:
    using stdoutLogAppenderPtr = std::shared_ptr<StdoutLogAppender>;
public:
    StdoutLogAppender();

    void Log(LogEvent::logEventPtr event) override;

    std::string ToYamlString() override;
};

class FileLogAppender : public LogAppender {
public:
    using fileLogAppenderPtr = std::shared_ptr<FileLogAppender>;

    FileLogAppender(const std::string &filePath);
    
    void Log(LogEvent::logEventPtr event) override;
    /**
     * @description: 重新打开文件
     * @return {*}
     */
    bool Reopen();

    std::string ToYamlString() override;

private:
    //文件路径
    std::string _fileName;
    //文件流
    std::ofstream _filestream;
    //上次重打开事件
    uint64_t _lastTime = 0;
    //文件打开打开错误标志
    bool _reopenError = false;

};

/// @brief 日志器类
class Logger {
public:
    using loggerPtr = std::shared_ptr<Logger>;
    using loggerSpinlock = Spinlock;
public:
    Logger(const std::string &name = "default");

    const std::string & GetName() const {return _name;}

    const uint64_t & GetCreateTime() const {return _createTime;}
    
    void SetLevel(LogLevel::Level level) {_level = level;}
    
    LogLevel::Level GetLevel() const {return _level;}

    void AddAppender(LogAppender::logAppenderPtr appender);

    void DelAppender(LogAppender::logAppenderPtr appender);

    void ClearAppender();

    void Log(LogEvent::logEventPtr logEvent);

    std::string ToYamlString();

private:
    loggerSpinlock _loggerSpinlock;
    //日志器名称 
    std::string _name;
    
    LogLevel::Level _level;
    //日志输出类集合
    std::list<LogAppender::logAppenderPtr> _appenders;

    uint64_t _createTime;
};

/// @brief 日志包装类 便于用于宏定义
class LogEventWrap{
public:
    LogEventWrap(Logger::loggerPtr logger, LogEvent::logEventPtr logEvent);
    /**
     * @description: 析构时日志输出
     * @return {*}
     */
    ~LogEventWrap();

    LogEvent::logEventPtr GetLogEvent() const {return _event;}

private:
    //日志类
    Logger::loggerPtr _logger;
    //日志事件
    LogEvent::logEventPtr _event;
};

/// @brief 日志管理类，专用单例
class LoggerManager {
public:
    using LoggerManagerSpinlock = Spinlock;

public:
    LoggerManager();
    //加载配置文件待实现
    void init();

    Logger::loggerPtr GetLogger(const std::string &name);

    Logger::loggerPtr GetRoot() {return _root;}

    std::string ToYamlString();

private:
    LoggerManagerSpinlock _loggrManagerSpinlock;
    //日志器集合
    std::map<std::string, Logger::loggerPtr> _loggers;
    //root 日志器
    Logger::loggerPtr _root;
};


using singleLoggerManager = MySylar::Singleton<LoggerManager>;

}

#endif