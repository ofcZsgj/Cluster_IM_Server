#include "user_module.hpp"
#include <iostream>

using namespace std;

// 向user表增加数据的方法
bool UserModule::insert(User &user)
{
    // 检查是否重复插入相同的name
    if (queryUsername(user.getName()) == true)
    {
        return false;
    }

    char sql[256] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPassword().c_str(), "offline");

    shared_ptr<Connection> sp = dbpool->getConnection();
    if (sp->update(sql))
    {
        // 获取插入成功的用户数据生成的主键自增ID
        user.setId(mysql_insert_id(sp->getMySQLConnection()));
        return true;
    }

    return false;
}

// 根据用户ID查询用户信息
User UserModule::query(int id)
{
    char sql[256] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    shared_ptr<Connection> sp = dbpool->getConnection();
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        // 获取一行数据
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPassword(row[2]);
            user.setState(row[3]);
            // 释放资源
            mysql_free_result(res);
            return user;
        }
    }

    return User();
}

// 更新用户的状态信息
bool UserModule::updateState(User user)
{
    char sql[256] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d",
            user.getState().c_str(), user.getId());

    shared_ptr<Connection> sp = dbpool->getConnection();
    if (sp->update(sql))
    {
        return true;
    }

    return false;
}

// 重置用户的状态信息
void UserModule::resetState()
{
    char sql[256] = {"update user set state = 'offline' where state = 'inline'"};

    shared_ptr<Connection> sp = dbpool->getConnection();
    sp->update(sql);

    return;
}

// 查询用户名是否重复
bool UserModule::queryUsername(string username)
{
    char sql[256] = {0};
    sprintf(sql, "select name from user where name = '%s'", username.c_str());

    shared_ptr<Connection> sp = dbpool->getConnection();
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        // 获取一行数据
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            if (row[0] != "")
            {
                return true;
            }
            // 释放资源
            mysql_free_result(res);
        }
    }

    return false;
}