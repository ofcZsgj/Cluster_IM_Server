#ifndef USER_MODULE_H
#define USER_MODULE_H

#include "user.hpp"

// user表的数据操作类
class UserModule
{
public:
    // 向user表增加数据的方法
    bool insert(User &user);
};

#endif