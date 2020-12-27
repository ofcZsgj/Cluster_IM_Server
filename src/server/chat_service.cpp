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
    string name = js["name"];
    string password = js["password"];

    User user = _userModule.query(js["id"].get<int>());
    if (user.getName() == name && user.getPassword() == password)
    {
        if (user.getState() == "offline")
        {
            // 登陆成功 更改用户状态为 inline
            user.setState("inline");
            // 通过module修改数据库状态信息
            _userModule.updateState(user);

            json success_response;
            success_response["msgid"] = LOGIN_MSG_ACK;
            success_response["errno"] = 0;
            success_response["id"] = user.getId();
            success_response["name"] = user.getName();
            conn->send(success_response.dump());
        }
        else
        {
            // 重复登陆
            json repeat_response;
            repeat_response["msgid"] = LOGIN_MSG_ACK;
            repeat_response["errno"] = 1;
            repeat_response["errmsg"] = "该账号已经登陆，请重新输入新账号";
            conn->send(repeat_response.dump());
        }
    }
    else
    {
        // 用户名或密码错误
        json repeat_response;
        repeat_response["msgid"] = LOGIN_MSG_ACK;
        repeat_response["errno"] = 2;
        repeat_response["errmsg"] = "用户名或密码错误，请重新输入";
        conn->send(repeat_response.dump());
    }
}

// 处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];

    // 创建一个user对象，将传入的json数据中的信息读出写入到对象中，再插入到数据库中
    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = _userModule.insert(user);

    // 判断是否插入成功
    if (state)
    {
        // 注册成功
        json success_response;
        success_response["msgid"] = REG_MSG_ACK;
        success_response["errno"] = 0;
        success_response["id"] = user.getId();
        conn->send(success_response.dump());
    }
    else
    {
        // 注册失败
        json fail_response;
        fail_response["msgid"] = REG_MSG_ACK;
        fail_response["errno"] = 1;
        conn->send(fail_response.dump());
    }
}