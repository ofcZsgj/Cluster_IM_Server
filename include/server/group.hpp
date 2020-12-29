#ifndef GROUP_H
#define GROUP_H

#include "group_user.hpp"
#include <vector>
// all_group表的ORM类，额外为处理业务增加了vector<GroupUser>字段记录群组人员信息
class Group
{
public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getId() { return this->id; }
    string getName() { return this->name; }
    string getPassword() { return this->desc; }
    // 返回该群组中人员列表的引用
    vector<GroupUser> &getUsers() { return this->users; }

private:
    int id;
    string name;
    string desc;
    // 每个群组中的人员信息
    vector<GroupUser> users;
};

#endif