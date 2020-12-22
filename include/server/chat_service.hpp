#ifndef CHAT_SERVICE_H
#define CHAT_SERVECE_H

#include <muduo/net/TcpConnection.h>
#include <functional>
#include <unordered_map>

#include "json.hpp"
using json = nlohmann::json;

using namespace muduo;
using namespace muduo::net;

using MsgHandler = std::function<void(const TcpConnectionPtr &conn,
                                      json &js, Timestamp time)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService *instance();
    // 处理登陆业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

private:
    ChatService();
    // 存储消息种类的ID和对应的业务映射方法
    std::unordered_map<int, MsgHandler> _msgHandlerMap;
};

#endif