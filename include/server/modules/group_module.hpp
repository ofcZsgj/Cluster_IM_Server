#ifndef GROUP_MODULE_H
#define GROUP_MODULE_H

#include "group.hpp"
#include "connection_pool.hpp"

extern ConnectionPool *dbpool;

// 维护群组信息的操作接口方法
class GroupModule
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    void addGroup(int userid, int groupid, string role);
    // 查询用户所在的所有群组的信息
    vector<Group> queryGroup(int userid);
    // 根据指定的groupid查询该群组的所有成员除userid外发送消息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif