#ifndef CHAT_SERVICE_H
#define CHAT_SERVECE_H

#include "user_module.hpp"
#include "offline_msg_module.hpp"
#include "friend_module.hpp"
#include "group_module.hpp"
#include "redis.hpp"
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
    // 处理一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 发送群聊消息业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理登出业务
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void ClientCloseException(const TcpConnectionPtr &conn);
    // 处理服务器端异常退出, 服务器异常，业务重置
    void reset();
    // 从redis消息队列中获取订阅的消息的回调函数
    void handleRedisSubscribeMessage(int, std::string);

private:
    ChatService();

    // 存储消息种类的ID和对应的业务映射方法
    std::unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;
    // 定义互斥锁保证_userConnMap的线程安全
    mutex _connMutex;

    /**
     * MySQL数据库的操作类对象
     * 1. user表数据操作类对象
     * 2. offline_message表的数据对象操作类
     * 3. all_group, group_user表的数据操作类
    */
    UserModule _userModule;
    OfflineMsgModule _offlineMsgModule;
    FriendModule _friendModule;
    GroupModule _groupModule;

    // redis操作对象
    Redis _redis;
};

#endif