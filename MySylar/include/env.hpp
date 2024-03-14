/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-28 18:03:24
 * @LastEditors: LOSER
 * @LastEditTime: 2024-01-28 23:41:51
 */
#ifndef __MYSYLAR_ENV_HPP__
#define __MYSYLAR_ENV_HPP__

#include <map>
#include <vector>

#include "Singleton.hpp"
#include "mutex.hpp"

namespace MySylar {

class EnV{
public:
    using envRWMutex = RWMutex;

public:
    /**
     * @brief 初始化，包括记录程序名称与路径，解析命令行选项和参数
     * @details 命令行选项全部以-开头，后面跟可选参数，选项与参数构成key-value结构，被存储到程序的自定义环境变量中，
     * 如果只有key没有value，那么value为空字符串
     * @param[in] argc main函数传入
     * @param[in] argv main函数传入
     * @return  是否成功
     */
    bool Init(int argc, char ** argv);

    void Add(const std::string & key, const std::string &val);

    bool Have(const std::string &key);

    void Del(const std::string &key);

    std::string Get(const std::string &key, const std::string &default_value = "");

    void AddHelp(const std::string &key, const std::string &desc);

    void RemoveHelp(const std::string &key);

    void PrintHelp();

    const std::string & GetExePath() const {return _exePath;}

    const std::string & GetCurPath() const {return _curPath;}

    bool SetEnV(const std::string &key, const std::string &val);

    std::string GetEnv(const std::string &key, const std::string &default_value = " ");

    std::string GetAbsolutePath(const std::string &path) const;

    std::string GetAbsoluteWorkPath(const std::string & path) const;

    std::string GetConfigPath();

private:
    envRWMutex _envRWMutex;

    std::map<std::string ,std::string> _args;

    std::vector<std::pair<std::string, std::string>> _helps;

    std::string _programName;

    std::string _exePath;

    std::string _curPath;
};

using singelEnv = MySylar::Singleton<EnV>;

}


#endif