/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-01 21:08:36
 * @LastEditors: LOSER
 * @LastEditTime: 2024-02-01 22:57:12
 */
#include "env.hpp"
#include "config.hpp"

MySylar::Logger::loggerPtr test = LOG_ROOT();

MySylar::EnV * env = MySylar::singelEnv::GetInstance();

int main(int argc, char * argv[]) {
    env->AddHelp("h", "print this help message");

    bool is_print_help = false;

    if(!env->Init(argc, argv)) {
        is_print_help = true;
    }

    if(env->Have("h")) {
        is_print_help = true;
    }

    if(is_print_help) {
        env->PrintHelp();
        return false;
    }

    INFO_LOG(test)<< "exe: " << env->GetExePath();
    INFO_LOG(test) <<"cwd: " << env->GetCurPath();
    INFO_LOG(test) << "absoluth path of test: " << env->GetAbsolutePath("test");
    INFO_LOG(test) << "conf path:" << env->GetConfigPath();

    env->Add("key1", "value1");
    INFO_LOG(test) << "key1: " << env->Get("key1");

    env->SetEnV("key2", "value2");
    INFO_LOG(test) << "key2: " << env->GetEnv("key2");

    INFO_LOG(test) << env->GetEnv("PATH");

}