/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-27 14:58:48
 * @LastEditors: LOSER
 * @LastEditTime: 2024-02-18 02:46:09
 */

#ifndef __OTHER_HPP__
#define __OTHER_HPP__

#pragma once

#include <time.h>
#include <cxxabi.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <execinfo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace MySylar {

__uint64_t GetElapsedMS();

pid_t GetThreadId();

pid_t GetFiberId();

std::string GetThreadName();

/**
 * @brief 设置线程名称，参考pthread_setname_np(3)
 * @note 线程名称不能超过16字节，包括结尾的'\0'字符
 */
void SetThreadName(const std::string &name);

/**
 * @brief 获取当前的调用栈
 * @param[out] bt 保存调用栈
 * @param[in] size 最多返回层数
 * @param[in] skip 跳过栈顶的层数
 */
void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

/**
 * @brief 获取当前栈信息的字符串
 * @param[in] size 栈的最大层数
 * @param[in] skip 跳过栈顶的层数
 * @param[in] prefix 栈信息前输出的内容
 */
std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");


/**
 * @brief 获取T类型的类型字符串
 */
template <class T>
const char *TypeToName() {
    static const char *s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}


/**
 * @brief 文件系统操作类
 */
class FSUtil {
public:
    /**
     * @brief 递归列举指定目录下所有指定后缀的常规文件，如果不指定后缀，则遍历所有文件，返回的文件名带路径
     * @param[out] files 文件列表 
     * @param[in] path 路径
     * @param[in] subfix 后缀名，比如 ".yml"
     */
    static void ListAllFile(std::vector<std::string> &files, const std::string &path, const std::string &subfix);

    /**
     * @brief 创建路径，相当于mkdir -p
     * @param[in] dirname 路径名
     * @return 创建是否成功
     */
    static bool Mkdir(const std::string &dirname);

    /**
     * @brief 判断指定pid文件指定的pid是否正在运行，使用kill(pid, 0)的方式判断
     * @param[in] pidfile 保存进程号的文件
     * @return 是否正在运行
     */
    static bool IsRunningPidfile(const std::string &pidfile);

    /**
     * @brief 删除文件或路径
     * @param[in] path 文件名或路径名 
     * @return 是否删除成功
     */
    static bool Rm(const std::string &path);

    /**
     * @brief 移动文件或路径，内部实现是先Rm(to)，再rename(from, to)，参考rename
     * @param[in] from 源
     * @param[in] to 目的地
     * @return 是否成功
     */
    static bool Mv(const std::string &from, const std::string &to);

    /**
     * @brief 返回绝对路径，参考realpath(3)
     * @details 路径中的符号链接会被解析成实际的路径，删除多余的'.' '..'和'/'
     * @param[in] path 
     * @param[out] rpath 
     * @return  是否成功
     */
    static bool Realpath(const std::string &path, std::string &rpath);

    /**
     * @brief 创建符号链接，参考symlink(2)
     * @param[in] from 目标 
     * @param[in] to 链接路径
     * @return  是否成功
     */
    static bool Symlink(const std::string &from, const std::string &to);

    /**
     * @brief 删除文件，参考unlink(2)
     * @param[in] filename 文件名
     * @param[in] exist 是否存在
     * @return  是否成功
     * @note 内部会判断一次是否真的不存在该文件
     */
    static bool Unlink(const std::string &filename, bool exist = false);

    /**
     * @brief 返回文件，即路径中最后一个/前面的部分，不包括/本身，如果未找到，则返回filename
     * @param[in] filename 文件完整路径
     * @return  文件路径
     */
    static std::string Dirname(const std::string &filename);

    /**
     * @brief 返回文件名，即路径中最后一个/后面的部分
     * @param[in] filename 文件完整路径
     * @return  文件名
     */
    static std::string Basename(const std::string &filename);

    /**
     * @brief 以只读方式打开
     * @param[in] ifs 文件流
     * @param[in] filename 文件名
     * @param[in] mode 打开方式
     * @return  是否打开成功
     */
    static bool OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode);

    /**
     * @brief 以只写方式打开
     * @param[in] ofs 文件流
     * @param[in] filename 文件名
     * @param[in] mode 打开方式
     * @return  是否打开成功
     */
    static bool OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode);
};



    
} // namespace MySylar

#endif
