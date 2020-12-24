#include "user_module.hpp"
#include "mysql.hpp"
#include <iostream>

using namespace std;

bool UserModule::insert(User &user)
{
    // 组装SQL语句
    char sql[128] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());

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