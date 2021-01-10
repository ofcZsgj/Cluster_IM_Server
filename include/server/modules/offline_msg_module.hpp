#ifndef OFFLINE_MSG_MODULE_H
#define OFFLINE_MSG_MODULE_H

#include <string>
#include <vector>
#include "connection_pool.hpp"

extern ConnectionPool *dbpool;
using namespace std;

// 提供离线消息表offline_message的操作接口方法
class OfflineMsgModule
{
public:
    // 向offline_message表中存储离线信息
    void insert(int userid, string msg);
    // 删除offline_message表中存储的离线消息
    void remove(int userid);
    // 查询用户的离线消息
    vector<string> query(int userid);
};

#endif