#include "group_module.hpp"
#include "mysql.hpp"

// 创建群组
bool GroupModule::createGroup(Group &group)
{
    char sql[128] = {0};
    sprintf(sql, "insert into all_group(group_name, group_desc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的群组数据生成的主键自增ID
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组
void GroupModule::addGroup(int userid, int groupid, string role)
{
    char sql[128] = {};
    sprintf(sql, "insert into group_user(user_id, group_id, group_role) values(%d, %d, '%s')", userid, groupid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
    return;
}

// 查询用户所在的所有群组的信息
vector<Group> GroupModule::queryGroup(int userid)
{
    char sql[256] = {0};
    // 查询所有的群组信息
    sprintf(sql, "select ta.id, ta.group_desc, ta.group_name from all_group as ta inner join group_user as tb on ta.id = tb.group_id where tb.user_id = %d", userid);

    MySQL mysql;
    std::vector<Group> groupvec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);

        if (res != nullptr)
        {
            // 把userid用户所在的全部群信息放到vector中
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupvec.push_back(group);
            }
            // 释放资源
            mysql_free_result(res);
        }
    }

    // 查询各个群组中的用户信息
    for (Group &group : groupvec)
    {
        char sql[256] = {0};
        // 查询每个群组中所有用户的id，name，state以及在群组的身份信息
        sprintf(sql, "select ta.id, ta.name, ta.state, tb.group_role from user as ta inner join group_user as tb on ta.id = tb.user_id where tb.group_id = %d", group.getId());

        MySQL mysql;
        if (mysql.connect())
        {
            MYSQL_RES *res = mysql.query(sql);

            if (res != nullptr)
            {
                // 把用户信息查询放到group_user的vector中，最后再放到group中
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser gu;
                    gu.setId(atoi(row[0]));
                    gu.setName(row[1]);
                    gu.setState(row[2]);
                    gu.setRole(row[3]);
                    group.getUsers().push_back(gu);
                }
                // 释放资源
                mysql_free_result(res);
            }
        }
    }

    return groupvec;
}

// 根据指定的groupid查询该群组的所有成员除userid外，返回这些用户的id给service来发送消息
vector<int> GroupModule::queryGroupUsers(int userid, int groupid)
{
    char sql[128] = {0};
    sprintf(sql, "select user_id from group_user where group_id = %d and user_id != %d", groupid, userid);

    vector<int> idvec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);

        if (res != nullptr)
        {
            // 把用户id存到vec中返回给service派发消息
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idvec.push_back(atoi(row[0]));
            }
            // 释放资源
            mysql_free_result(res);
        }
    }

    return idvec;
}