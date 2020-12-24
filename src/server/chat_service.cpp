#include "chat_service.hpp"
#include "public.hpp"

#include <muduo/base/Logging.h>

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的回调函数
ChatService::ChatService()
{
    _msgHandlerMap.insert(make_pair(LOGIN_MSG,
                                    std::bind(&ChatService::login, this, _1, _2, _3)));
    _msgHandlerMap.insert(make_pair(REG_MSG,
                                    std::bind(&ChatService::reg, this, _1, _2, _3)));
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp) {
            LOG_ERROR << "msgid:" << msgid << "can not find handler";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 处理登陆业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do login service";
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    LOG_INFO << "do reg service";
}