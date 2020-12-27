#include "chat_service.hpp"
#include "public.hpp"

#include <string>
#include <vector>
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
    // 登陆
    _msgHandlerMap.insert(make_pair(LOGIN_MSG,
                                    std::bind(&ChatService::login, this, _1, _2, _3)));
    // 注册
    _msgHandlerMap.insert(make_pair(REG_MSG,
                                    std::bind(&ChatService::reg, this, _1, _2, _3)));
    // 一对一聊天
    _msgHandlerMap.insert(make_pair(ONE_CHAT_MSG,
                                    std::bind(&ChatService::oneChat, this, _1, _2, _3)));
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

// 处理服务器端异常退出, 服务器异常，业务重置
void ChatService::reset()
{
    // 服务器异常退出时将所有inline的用户信息重置为offline
    _userModule.resetState();
}

// 处理登陆业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string password = js["password"];
    int id = js["id"].get<int>();

    User user = _userModule.query(id);
    if (user.getName() == name && user.getPassword() == password)
    {
        if (user.getState() == "offline")
        {
            {
                //登陆成功，记录用户的长连接信息
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert(make_pair(user.getId(), conn));
            }
            // 登陆成功 更改用户状态为 inline
            user.setState("inline");
            // 通过module修改数据库状态信息
            _userModule.updateState(user);

            json success_response;
            success_response["msgid"] = LOGIN_MSG_ACK;
            success_response["errno"] = 0;
            success_response["id"] = user.getId();
            success_response["name"] = user.getName();

            // 查询用户是否有离线消息
            std::vector<std::string> vec = _offlineMsgModule.query(id);
            if (!vec.empty())
            {
                success_response["offlinemsg"] = vec;
                // 读取用户的离线消息后，删除该用户在offline_message表中的离线消息
                _offlineMsgModule.remove(id);
            }
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

// 处理客户端异常退出
void ChatService::ClientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                // 从unordder_map表中删除用户的连接信息
                _userConnMap.erase(it);
            }
        }
    }
    // 设置用户的状态为offline
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModule.updateState(user);
    }
}

// 处理一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息：服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // toid用户不在线，存储离线消息
    _offlineMsgModule.insert(toid, js.dump());
}