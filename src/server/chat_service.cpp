#include "chat_service.hpp"
#include "public.hpp"
#include "user.hpp"

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
    /**
     * 用户基本业务管理相关事件处理回调注册
    */
    // 登陆
    _msgHandlerMap.insert(make_pair(LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)));
    // 登出
    _msgHandlerMap.insert(make_pair(LOGINOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)));
    // 注册
    _msgHandlerMap.insert(make_pair(REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)));
    // 一对一聊天
    _msgHandlerMap.insert(make_pair(ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)));
    // 添加好友
    _msgHandlerMap.insert(make_pair(ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)));

    /**
     *  群组业务管理相关事件处理回调注册
    */
    // 创建群组
    _msgHandlerMap.insert(make_pair(CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)));
    // 加入群组
    _msgHandlerMap.insert(make_pair(ADD_GROUP_MSG, std::bind(&ChatService::joinGroup, this, _1, _2, _3)));
    // 群聊
    _msgHandlerMap.insert(make_pair(GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)));

    // 连接redis
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
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
    int id = js["id"].get<int>();
    string password = js["password"];

    User user = _userModule.query(id);
    if (user.getId() == id && user.getPassword() == password)
    {
        if (user.getState() == "offline")
        {
            {
                // 登陆成功，记录用户的长连接信息
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert(make_pair(user.getId(), conn));
            }

            // 登陆成功 向redis订阅channel(id)
            _redis.subscribe(id);

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

            // 查询用户好友信息
            std::vector<User> frivec = _friendModule.query(id);

            if (!frivec.empty())
            {
                std::vector<std::string> vec2;
                for (User &user : frivec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                success_response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModule.queryGroup(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                success_response["groups"] = groupV;
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

    // 用户下线，在redis中退订channel
    _redis.unsubscribe(user.getId());

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
    int toid = js["toid"].get<int>();

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

    // 若不在当前服务器 则查询toid是否在线 在线即通过redis推送到todi订阅的的channel中
    User user = _userModule.query(toid);
    if (user.getState() == "inline")
    {
        _redis.publish(toid, js.dump());
        return;
    }
    else
    {
        // toid用户不在线，存储离线消息
        _offlineMsgModule.insert(toid, js.dump());
    }
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //存储好友信息(后期可以优化将好友信息存储在客户端)
    _friendModule.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);

    // 创建成功，该用户添加至该群组中，role为admin
    if (_groupModule.createGroup(group))
    {
        _groupModule.addGroup(userid, group.getId(), "admin");
    }
}

// 加入群组业务
void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModule.addGroup(userid, groupid, "normal");
}

// 发送群聊消息业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    // 得到这个用户所在的群成员id列表
    vector<int> idvec = _groupModule.queryGroupUsers(userid, groupid);

    // 由于多个worker线程同时会处理各类消息的事件回调方法，使用到这个记录用户连接的unordered_map表
    // 因此访问时需要加锁保证线程安全
    lock_guard<mutex> lock(_connMutex);
    for (int id : idvec)
    {
        // unordered_map查询这个id的连接TcpConnectionPtr是否存在当前服务器，存在直接推送
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 这条连接在线，转发消息：服务器主动推送消息给这条连接的用户
            it->second->send(js.dump());
            continue;
        }
        else
        {
            // 若不在当前服务器 则查询toid是否在线 在线即通过redis推送到todi订阅的的channel中
            User user = _userModule.query(id);
            if (user.getState() == "inline")
            {
                _redis.publish(id, js.dump());
                return;
            }
            else
            {
                // 用户不在线，存储离线群聊消息，下次登陆时推送
                _offlineMsgModule.insert(id, js.dump());
            }
        }
    }
}

// 处理登出业务
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        // 加锁后在map中删除用户连接
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户下线，在redis中退订channel
    _redis.unsubscribe(userid);

    // 设置用户为offline
    User user(userid, "", "", "offline");
    _userModule.updateState(user);
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModule.insert(userid, msg);
}