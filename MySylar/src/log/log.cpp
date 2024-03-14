/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-18 14:43:45
 * @LastEditors: LOSER
 * @LastEditTime: 2024-01-28 22:01:18
 */
#include "log.hpp"

namespace MySylar {

const char *LogLevel::ToString(LogLevel::Level level)
{
    switch (level) 
    {
    case LogLevel::FATAL:
        return "FATAL";
    case LogLevel::ALERT:
        return "ALERT";
    case LogLevel::CRIT:
        return "CRIT";
    case LogLevel::ERROR:
        return "ERROR";
    case LogLevel::WARN:
        return "WARN";
    case LogLevel::NOTICE:
        return "NOTICE";
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::DEBUG:
        return "DEBUG";
    default:
        return "NOTSET";
    }

    return "NOTSET";
}

LogLevel::Level LogLevel::FromString(const std::string &str)
{
    #define RET(level, type) if(str == #type) {return LogLevel::level;}
    RET(FATAL, fatal);
    RET(ALERT, alert);
    RET(CRIT, crit);
    RET(ERROR, error);
    RET(WARN, warn);
    RET(NOTICE, notice);
    RET(INFO, info);
    RET(DEBUG, debug);

    RET(FATAL, FATAL);
    RET(ALERT, ALERT);
    RET(CRIT, CRIT);
    RET(ERROR, ERROR);
    RET(WARN, WARN);
    RET(NOTICE, NOTICE);
    RET(INFO, INFO);
    RET(DEBUG, DEBUG);

    #undef RET

    return LogLevel::NOTSET;
}

LogEvent::LogEvent(const std::string &loggerName, LogLevel::Level elevel, const char *fileName, 
    int32_t lineNum, int64_t elapseTime, uint32_t threadId, uint64_t fiberId, time_t timeT, 
        const std::string &threadName)
:_eLevel(elevel)
,_fileName(fileName)
,_lineNum(lineNum)
,_elapseTime(elapseTime)
,_threadId(threadId)
,_fiberId(fiberId)
,_timeT(timeT)
,_threadName(threadName)
,_loggerName(loggerName)
{

}
void LogEvent::Printf(const char *fmt, ...) {
    //va_list 处理可变参数
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void LogEvent::Vprintf(const char *fmt, va_list ap) {
    char *buf = nullptr;
    //vasprintf 是一个 C 库函数，它可以通过可变参数创建一个格式化的字符串，
    //并将其存储在动态分配的内存中。它的使用方法与 printf 类似，但它不会将结果打印到标准输出流中，
    //而是将其存储在一个指向字符数组的指针中。
    int len = vasprintf(&buf, fmt, ap);
    if(len != -1) {
        _ssContent << std::string(buf, len);
        free(buf);
    }
}
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << event->GetContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << LogLevel::ToString(event->GetLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << event->GetElapse();
    }
};

class LoggerNameFormatItem : public LogFormatter::FormatItem {
public:
    LoggerNameFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << event->GetLoggerName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << event->GetThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << event->GetFiberId();
    }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
public:
    ThreadNameFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << event->GetThreadName();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void Format(std::ostream& os, LogEvent::logEventPtr event) override {
        struct tm tm;
        time_t time = event->GetTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem {
public:
    FileNameFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << event->GetFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << event->GetLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :_string(str) {}
    void Format(std::ostream& os, LogEvent::logEventPtr event) override {
        os << _string;
    }
private:
    std::string _string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << "\t";
    }
};

class PercentSignFormatItem : public LogFormatter::FormatItem {
public:
    PercentSignFormatItem(const std::string &str) {}
    void Format(std::ostream &os, LogEvent::logEventPtr event) override {
        os << "%";
    }
};

LogFormatter::LogFormatter(const std::string &pattern)
:_pattern(pattern)
{
    Init();
}

void LogFormatter::Init()
{
    std::vector<std::pair<int, std::string>> patterns;

    std::string tmp;

    std::string dateFormat;

    bool error = false;

    bool parsing_string = true;//解析常规为true 解析模板字符为false

    size_t i = 0;
    //%d{%Y-%m-%d %H:%M:%S} [%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
    while(i < _pattern.size()) {
        std::string s = std::string(1, _pattern[i]);
        if(s == "%") {
            if(parsing_string) {
                if(!tmp.empty()) {
                    patterns.emplace_back(std::make_pair(0, tmp));
                }
                tmp.clear();
                parsing_string = false;

                i++;
                continue;
            } else {
            patterns.emplace_back(std::make_pair(1, s));
            parsing_string = true;

            i++;
            continue;
            }
        } else {
           if(parsing_string) {
                tmp += s;
                i++;
                continue;
           } else {
                patterns.emplace_back(std::make_pair(1, s));
                parsing_string = true;

                if(s != "d") {
                    i++;
                    continue;
                }
                i++;
                if(i < _pattern.size() && _pattern[i] != '{') {
                    continue;
                }
                i++;
                while(i < _pattern.size() && _pattern[i] != '}') {
                    dateFormat.push_back(_pattern[i]);
                    i++;
                }
                if(_pattern[i] != '}') {
                    std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << _pattern << "] '{' not closed" << std::endl;
                    error = true;
                    break;
                }
                i++;
                continue;
           }

        }
        
    }

    if(error) {
        _isError = true;
        return;
    }

    if(!tmp.empty()) {
        patterns.emplace_back(std::make_pair(0, tmp));
        tmp.clear();
    }
#define FF(str, S) {#str, [](const std::string& fmt) {return FormatItem::formatItemPtr(new S(fmt));}}
    static std::map<std::string, std::function<FormatItem::formatItemPtr(const std::string & str)>> staticFormatIteams = {

         //FormatItem 派生出不同功能的子类
         FF(m, MessageFormatItem),
         FF(p, LevelFormatItem),
         FF(c, LoggerNameFormatItem),
         FF(r, ElapseFormatItem),
         FF(f, FileNameFormatItem),
         FF(l, LineFormatItem),
         FF(t, ThreadIdFormatItem),
         FF(F, FiberIdFormatItem),
         FF(N, ThreadNameFormatItem),
         FF(%, PercentSignFormatItem),
         FF(T, TabFormatItem),
         FF(n, NewLineFormatItem),

    };
#undef FF

    for(auto &v : patterns) {
        if(v.first == 0) {
            _formatItems.emplace_back(FormatItem::formatItemPtr(new StringFormatItem(v.second)));
        }else if(v.second == "d") {
            _formatItems.emplace_back(FormatItem::formatItemPtr(new DateTimeFormatItem(dateFormat)));
        } else {
            auto it = staticFormatIteams.find(v.second);
            if(it == staticFormatIteams.end()) {
                std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << _pattern << "] " << 
                "unknown format item: " << v.second << std::endl;
                error = true;
                break;
            } else {
                _formatItems.emplace_back(it->second(v.second));
            }
        }
    }

    if(error) {
        _isError = true;
        return;
    }

}

std::string LogFormatter::Format(LogEvent::logEventPtr event)
{
    std::stringstream ss;
    for(auto &i:_formatItems) {
        i->Format(ss, event);
    }
    return ss.str();
}
std::ostream &LogFormatter::Format(std::ostream &os, LogEvent::logEventPtr event)
{
    for(auto &i:_formatItems) {
        i->Format(os, event);
    }
    return os;
}

LogAppender::LogAppender(LogFormatter::logFormatterPtr defaultFormatter)
: _defaultFormatter(defaultFormatter) {
}

void LogAppender::SetFormatter(LogFormatter::logFormatterPtr formatter)
{
    logAppenderSpinlock::lock lock(_appenderSpinlock);
    _formatter = formatter;
}

LogFormatter::logFormatterPtr LogAppender::GetFormatter()
{
    logAppenderSpinlock::lock lock(_appenderSpinlock);
    return _formatter ? _formatter : _defaultFormatter;
}

//?
StdoutLogAppender::StdoutLogAppender()
    : LogAppender(LogFormatter::logFormatterPtr(new LogFormatter)) {
}

void StdoutLogAppender::Log(LogEvent::logEventPtr event)
{
    if(_formatter) {

    }else {
        _defaultFormatter->Format(std::cout, event);
    }
}

std::string StdoutLogAppender::ToYamlString()
{
    logAppenderSpinlock::lock lock(_appenderSpinlock);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    node["pattern"] = _formatter->GetPattern();
    std::stringstream ss;
    ss << node;
    return ss.str();
}

FileLogAppender::FileLogAppender(const std::string &filePath)
:LogAppender(LogFormatter::logFormatterPtr(new LogFormatter)){
    _fileName = filePath;
    Reopen();
    if(_reopenError) {
        std::cout << "reopen file " << _fileName << " error" << std::endl;
    }
}
/**
 * 如果一个日志事件距离上次写日志超过3秒，那就重新打开一次日志文件
 */
void FileLogAppender::Log(LogEvent::logEventPtr event)
{
    uint64_t now = event->GetTime();
    if(now >= (_lastTime + 3)) {
        Reopen();
        if(_reopenError) {
            std::cout << "reopen file " << _fileName << " error" << std::endl;
        }
        _lastTime = now;
    }
    if(_reopenError) {
        return;
    }
    logAppenderSpinlock::lock lock(_appenderSpinlock);
    if(_formatter) {
        if(!_formatter->Format(_filestream, event)) {
            std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
        }
    } else {
        if(!_defaultFormatter->Format(_filestream, event)) {
            std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
        }
    }
}

bool FileLogAppender::Reopen() {
    logAppenderSpinlock::lock lock(_appenderSpinlock);
    if(_filestream) {
        _filestream.close();
    }
    _filestream.open(_fileName, std::ios::app);
    _reopenError = !_filestream;
    return !_reopenError;
}

std::string FileLogAppender::ToYamlString() {
    logAppenderSpinlock::lock lock(_appenderSpinlock);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = _fileName;
    node["pattern"] = _formatter ? _formatter->GetPattern() : _defaultFormatter->GetPattern();
    std::stringstream ss;
    ss << node;
    return ss.str();
}

Logger::Logger(const std::string &name)
:_name(name)
,_level(LogLevel::INFO)
,_createTime(0){
    
}

void Logger::AddAppender(LogAppender::logAppenderPtr appender)
{
    loggerSpinlock::lock lock(_loggerSpinlock);
    _appenders.emplace_back(appender);
}

void Logger::DelAppender(LogAppender::logAppenderPtr appender)
{
    loggerSpinlock::lock tmpLock(_loggerSpinlock);
    for(auto it = _appenders.begin(); it != _appenders.end(); ++it) {
        if(*it == appender) {
            _appenders.erase(it);
            break;
        }
    }
}

void Logger::ClearAppender()
{
    loggerSpinlock::lock tmpLock(_loggerSpinlock);
    _appenders.clear();
}

void Logger::Log(LogEvent::logEventPtr logEvent)
{
    if(logEvent->GetLevel() <= _level) {
        for(auto &ref : _appenders) {
           ref->Log(logEvent); 
        }
    }
}

std::string Logger::ToYamlString()
{
    loggerSpinlock::lock tmpLock(_loggerSpinlock);
    YAML::Node node;
    node["name"] = _name;
    node["level"] = LogLevel::ToString(_level);
    for(auto &ref : _appenders) {
        node["appenders"].push_back(YAML::Load(ref->ToYamlString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogEventWrap::LogEventWrap(Logger::loggerPtr logger, LogEvent::logEventPtr logEvent)
:_logger(logger)
,_event(logEvent) {
}

LogEventWrap::~LogEventWrap() {
    _logger->Log(_event);
}

LoggerManager::LoggerManager()
{
    _root.reset(new Logger("root"));
    _root->AddAppender(LogAppender::logAppenderPtr(new StdoutLogAppender));
    _loggers[_root->GetName()] = _root;
    init();
}

void LoggerManager::init()
{
}

/**
 * 如果指定名称的日志器未找到，那会就新创建一个，但是新创建的Logger是不带Appender的，
 * 需要手动添加Appender
 */
Logger::loggerPtr LoggerManager::GetLogger(const std::string &name)
{
    LoggerManagerSpinlock::lock tmpLock(_loggrManagerSpinlock);
    auto it = _loggers.find(name);
    if(it != _loggers.end()) {
        return it->second;
    }

    Logger::loggerPtr logger(new Logger(name));
    _loggers[name] = logger;
    return logger;
}

std::string LoggerManager::ToYamlString()
{
    LoggerManagerSpinlock::lock tmpLock(_loggrManagerSpinlock);
    YAML::Node node;
    for(auto& i : _loggers) {
        node.push_back(YAML::Load(i.second->ToYamlString()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

//
}