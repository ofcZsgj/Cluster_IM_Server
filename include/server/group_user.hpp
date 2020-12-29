#ifndef GROUP_USER_H
#define GROUP_USER_H

#include "user.hpp"

// user表加上一个的group_user表的group_role字段的ORM类，用于向业务层返回用户所处在的群组的人员详细信息
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

private:
    // 该用户的权限
    string role;
};

#endif