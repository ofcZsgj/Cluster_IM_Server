#ifndef FRIEND_MODULE_H
#define FRIEND_MODULE_H

#include <vector>
#include "user.hpp"
#include "connection_pool.hpp"

extern ConnectionPool *dbpool;

class FriendModule
{
public:
    // 添加好友关系
    void insert(int userid, int friendid);
    // 返回用户的好友列表
    std::vector<User> query(int userid);
};

#endif