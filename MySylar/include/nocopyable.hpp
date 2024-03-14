/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-21 19:54:18
 * @LastEditors: LOSER
 * @LastEditTime: 2024-01-21 19:58:02
 */
#ifndef _MYSYLAR_NOCOPYABLE_H__
#define _MYSYLAR_NOCOPYABLE_H__

namespace MySylar {


class Nocopyable {
public:

    Nocopyable() = default;

    ~Nocopyable() = default;

    Nocopyable(const Nocopyable&) = delete;

    Nocopyable& operator=(const Nocopyable&) = delete;
};
}


#endif