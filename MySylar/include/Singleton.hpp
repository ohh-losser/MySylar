/*
 * @Description: 
 * @Version: 1.0
 * @Author: LOSER
 * @Date: 2024-01-27 20:27:27
 * @LastEditors: LOSER
 * @LastEditTime: 2024-01-31 15:38:40
 */
//单例模板

#ifndef __SINGLETON__HPP__
#define __SINGLETON__HPP__

namespace MySylar {

template <typename T>
class Singleton {
    //使用默认构造和析构函数
    Singleton() = default;
    ~Singleton() = default;
public:
	//删除默认的移动、拷贝、赋值、取址
    Singleton(Singleton  &&) = delete;
    Singleton(const Singleton  &) = delete;
    void operator = (const Singleton  &) = delete;
    T *operator &() = delete;
	//获取实例，对此处有异议也可自行查阅资料
    static T* GetInstance()
    {
        static T object;
        return &object;
    }
};


}

#endif

