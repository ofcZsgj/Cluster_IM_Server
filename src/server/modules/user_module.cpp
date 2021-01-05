#include "user_module.hpp"
#include "mysql.hpp"
#include <iostream>

using namespace std;

// 向user表增加数据的方法
bool UserModule::insert(User &user)
{
    // 组装SQL语句
    char sql[128] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPassword().c_str(), "offline");

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的主键自增ID
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 根据用户ID查询用户信息
User UserModule::query(int id)
{
    // 组装SQL语句
    char sql[128] = {0};
    sprintf(sql, "select * from user where id = %d", id);

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);

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
    }

    return User();
}

// 更新用户的状态信息
bool UserModule::updateState(User user)
{
    // 组装SQL语句
    char sql[128] = {0};
    sprintf(sql, "update user set state = '%s' where id = %d",
            user.getState().c_str(), user.getId());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// 重置用户的状态信息
void UserModule::resetState()
{
    char sql[128] = {"update user set state = 'offline' where state = 'inline'"};

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
    return;
}