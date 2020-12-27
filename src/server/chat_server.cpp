#include "chat_server.hpp"
#include "chat_service.hpp"

#include <functional>
using namespace std;
using namespace std::placeholders;

#include "json.hpp"
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,               // 事件循环
                       const InetAddress &listenAddr, // IP + Port
                       const string &nameArg)         // 服务器的名字
    : _server(loop, listenAddr, nameArg),
      _loop(loop)

{
    // 给服务器注册用户连接和断开时的回调
    _server.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));

    // 给服务器注册用户用户读写事件的回调
    _server.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置服务端的线程数量 1个I/O线程 N-1个worker工作线程，muduo库会自动分配
    _server.setThreadNum(6);
}

// 开启事件循环
void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数
// typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 处理客户端连接中断
    if (!conn->connected())
    {
        ChatService::instance()->ClientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn, // 连接
                           Buffer *buffer,               // 缓冲区
                           Timestamp time)               // 接收到数据时的事件
{
    // 读出buffer中的数据
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);

    /**
     * ****************************************************
     * 通过js[msgid]获取 => 业务handle => conn js time......
     * 达到网络模块和业务模块的解耦
     * ****************************************************
     */
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器来执行相应的业务逻辑
    msgHandler(conn, js, time);
}