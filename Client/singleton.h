#ifndef SINGLETON_H
#define SINGLETON_H
#include "global.h"

template <typename T>
class Singleton{
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator= (const Singleton<T>& st) = delete;

    static std::shared_ptr<T> _instance;

public:
    static std::shared_ptr<T> GetInstance()
    {
        static std::once_flag s_flag;
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T);
            // 这里为什么不用make_shared？ 因为对T的构造是私有（继承）的，make_shared无权限
        });

        return _instance;
    }

    void PringAddress()
    {
        std::cout << _instance.get() << std::endl;
    }

    ~Singleton()
    {
        std::cout << "this is singleton destruct" << std::endl;
    }

};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;


#endif // SINGLETON_H
