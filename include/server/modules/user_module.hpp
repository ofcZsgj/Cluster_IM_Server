#ifndef USER_MODULE_H
#define USER_MODULE_H

#include "user.hpp"
#include "connection_pool.hpp"

extern ConnectionPool *dbpool;

// user表的数据操作类
class UserModule
{
public:
    // 向user表增加数据的方法
    bool insert(User &user);
    // 根据用户ID查询用户信息
    User query(int id);
    // 更新用户的状态信息
    bool updateState(User user);
    // 重置用户的状态信息
    void resetState();
    // 查询用户名是否重复
    bool queryUsername(string username);
};

#endif