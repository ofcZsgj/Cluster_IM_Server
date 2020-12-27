#ifndef CHAT_SERVICE_H
#define CHAT_SERVECE_H

#include "user_module.hpp"
#include <muduo/net/TcpConnection.h>
#include <functional>
#include <unordered_map>
#include <mutex>

#include "json.hpp"
using json = nlohmann::json;

using namespace muduo;
using namespace muduo::net;

// ！！！！！表示处理消息的事件回调方法类型！！！！！
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,
                                      json &js, Timestamp)>;

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
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void ClientCloseException(const TcpConnectionPtr &conn);

private:
    ChatService();

    // 存储消息种类的ID和对应的业务映射方法
    std::unordered_map<int, MsgHandler> _msgHandlerMap;
    // user表的数据对象操作类
    UserModule _userModule;
    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    // 定义互斥锁保证_userConnMap的线程安全
    mutex _connMutex;
};

#endif