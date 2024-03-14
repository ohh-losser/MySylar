/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-02-18 02:25:56
 * @LastEditors: LOSER
 * @LastEditTime: 2024-03-03 01:18:07
 */
#ifndef __MACRO_HPP__
#define __MACRO_HPP__

#include <string.h>
#include <assert.h>
#include "log.hpp"
#include "other.hpp"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define MYSYLAR_LIKELY(x) __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define MYSYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define MYSYLAR_LIKELY(x) (x)
#define MYSYLAR_UNLIKELY(x) (x)
#endif

/// 断言宏封装
#define MYSYLAR_ASSERT(x)                                                               \
    if (MYSYLAR_UNLIKELY(!(x))) {                                                         \
        ERROR_LOG(LOG_ROOT()) << "ASSERTION: " #x                                       \
                                          << "\nbacktrace:\n"                           \
                                          << MySylar::BacktraceToString(100, 2, "    ");\
        assert(x);                                                                      \
    }

/// 断言宏封装
#define MYSYLAR_ASSERT2(x, w)                                                            \
    if (MYSYLAR_UNLIKELY(!(x))) {                                                        \
        ERROR_LOG(LOG_ROOT()) << "ASSERTION: " #x                          \
                                          << "\n"                                      \
                                          << w                                         \
                                          << "\nbacktrace:\n"                          \
                                          << MySylar::BacktraceToString(100, 2, "    "); \
        assert(x);                                                                     \
    }

#endif