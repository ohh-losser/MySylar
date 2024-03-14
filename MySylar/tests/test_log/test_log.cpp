/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-19 13:34:01
 * @LastEditors: LOSER
 * @LastEditTime: 2024-02-01 20:52:28
 */
#include <log.hpp>
#include <config.hpp>
#include <env.hpp>

MySylar::Logger::loggerPtr testLog = LOG_ROOT();

int main(int argc, char * argv[]) {
    MySylar::singelEnv::GetInstance()->Init(argc, argv);
    std::string configDirPath = MySylar::singelEnv::GetInstance()->GetConfigPath();
    testLog->SetLevel(MySylar::LogLevel::DEBUG);
    DEBUG_LOG(testLog) << configDirPath;
    //std::cout << configDirPath << std::endl;
    MySylar::Config::LoadFromConfDir(configDirPath);


    MySylar::FileLogAppender::logAppenderPtr logFile(new MySylar::FileLogAppender("./log.txt"));
    testLog->AddAppender(logFile);

    DEBUG_LOG(testLog) << "test file log";

    
    return 0;
}

