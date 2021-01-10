#include "friend_module.hpp"

// 添加好友关系
void FriendModule::insert(int userid, int friendid)
{
    char sql[256] = {0};
    sprintf(sql, "insert into friend(user_id, friend_id) values(%d, %d)", userid, friendid);

    shared_ptr<Connection> sp = dbpool->getConnection();
    sp->update(sql);

    return;
}

// 返回用户的好友列表
std::vector<User> FriendModule::query(int userid)
{
    char sql[256] = {0};
    // user表和frieend联合查询，找到userid的所有好友
    sprintf(sql, "select ta.id, ta.name, ta.state from user as ta inner join friend as tb on ta.id = tb.friend_id where tb.user_id = %d", userid);

    // 存放用户的好友信息
    std::vector<User> vec;

    shared_ptr<Connection> sp = dbpool->getConnection();
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        // 把userid用户的所有好友信息放到vector中返回
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setState(row[2]);
            vec.push_back(user);
        }
        // 释放资源
        mysql_free_result(res);
    }

    return vec;
}