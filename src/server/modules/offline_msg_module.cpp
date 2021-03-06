#include "offline_msg_module.hpp"

// 向offline_message表中存储离线信息
void OfflineMsgModule::insert(int userid, string msg)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into offline_message values(%d, '%s')", userid, msg.c_str());

    shared_ptr<Connection> sp = dbpool->getConnection();
    sp->update(sql);

    return;
}

// 删除offline_message表中存储的离线消息
void OfflineMsgModule::remove(int userid)
{
    char sql[256] = {0};
    sprintf(sql, "delete from offline_message where user_id = %d", userid);

    shared_ptr<Connection> sp = dbpool->getConnection();
    sp->update(sql);

    return;
}

// 查询用户的离线消息
vector<string> OfflineMsgModule::query(int userid)
{
    char sql[256] = {0};
    sprintf(sql, "select message from offline_message where user_id = %d", userid);

    // 存放用户的所有离线消息
    vector<string> vec;

    shared_ptr<Connection> sp = dbpool->getConnection();
    MYSQL_RES *res = sp->query(sql);
    if (res != nullptr)
    {
        // 将userid的所有的离线消息放入到vec中返回
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            vec.push_back(row[0]);
        }
        mysql_free_result(res);
    }

    return vec;
}