/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-28 18:03:50
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-01 17:26:14
 */

#include<unistd.h>
#include<stdlib.h>
#include<string.h>

#include<iomanip>

#include "env.hpp"
#include "log.hpp"

#include "config.hpp"
namespace MySylar {

static MySylar::Logger::loggerPtr envLogger = LOG_NAME("SYSTEM");

bool MySylar::EnV::Init(int argc, char **argv)
{
    char link[1024] = {0};
    char path[1024] = {0};

    sprintf(link, "/proc/%d/exe", getpid());//获取当前进程的识别码
    readlink(link, path, sizeof(path));

    _exePath = path;

    auto pos = _exePath.find_last_of("/");
    _curPath = _exePath.substr(0, pos) + "/";

    _programName = argv[0];

    const char * nowKey = nullptr;

    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            if(strlen(argv[i]) > 1) {
                if(nowKey) {
                    Add(nowKey, "");
                }
                nowKey = argv[i] + 1;
            } else {
                ERROR_LOG(envLogger) << "invalid arg idx=" << i
                            << " val=" << argv[i];
                return false;
            }
        } else {
            if(nowKey) {
                Add(nowKey, argv[i]);
                nowKey = nullptr;
            } else {
                ERROR_LOG(envLogger) << "invalid arg idx=" << i
                            << " val=" << argv[i];
                return false;
            }
        }
    }
    if(nowKey) {
        Add(nowKey, "");
    }

    return true;
}

void EnV::Add(const std::string &key, const std::string &val)
{
    envRWMutex::WriteLock lock(_envRWMutex);
    _args[key] = val;
}
bool EnV::Have(const std::string &key)
{
    envRWMutex::WriteLock lock(_envRWMutex);
    return _args.find(key) != _args.end();
}
void EnV::Del(const std::string &key)
{
    envRWMutex::WriteLock lock(_envRWMutex);
    _args.erase(key);
}

std::string EnV::Get(const std::string &key, const std::string &default_value)
{
    envRWMutex::WriteLock lock(_envRWMutex);
    auto it = _args.find(key);
    return it != _args.end() ? it->second : default_value;
}
void EnV::AddHelp(const std::string &key, const std::string &desc)
{
    RemoveHelp(key);
    envRWMutex::WriteLock lock(_envRWMutex);
    _helps.emplace_back(std::make_pair(key, desc));
}
void EnV::RemoveHelp(const std::string &key)
{
    envRWMutex::WriteLock lock(_envRWMutex);
    for(auto it = _helps.begin(); it != _helps.end();) {
        if(it->first == key) {
            it = _helps.erase(it);
        }else {
            ++it;
        }
    }
}
void EnV::PrintHelp()
{
    envRWMutex::WriteLock lock(_envRWMutex);
    INFO_LOG(envLogger) << "Usage: " << _programName << " [options]";
    for(auto &i :_helps) {
        INFO_LOG(envLogger) << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
    }
}
bool EnV::SetEnV(const std::string &key, const std::string &val)
{
    return setenv(key.c_str(), val.c_str(), 1);
}
std::string EnV::GetEnv(const std::string &key, const std::string &default_value)
{
    const char * v = getenv(key.c_str());
    if(v == nullptr) {
        return default_value;
    } else {
        return v;
    }
}
std::string EnV::GetAbsolutePath(const std::string &path) const
{
    if(path.empty()) {
        return "/";
    }
    if(path[0] == '/') {
        return path;
    }
    return _curPath + path;
}
std::string EnV::GetAbsoluteWorkPath(const std::string &path) const
{
    if(path.empty()) {
        return "/";
    }
    if(path[0] == '/') {
        return path;
    }
    static MySylar::ConfigVar<std::string>::configVarPtr g_server_work_path =
        MySylar::Config::Lookup<std::string>("server.work_path");
    return g_server_work_path->GetValue() + "/" + path;
}
std::string EnV::GetConfigPath()
{
    return GetAbsolutePath(Get("c", "conf.ini"));
}
}
