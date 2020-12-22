#ifndef CHAT_SERVER_H
#define CHAT_SERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

// 聊天服务器的主类
class ChatServer
{
public:
    // 初始化muduo/net中的TcpServer
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP + Port
               const string &nameArg);        // 服务器的名字

    // 开启事件循环
    void start();

private:
    // 上报连接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &conn);

    // 上报读写事件相关的回调函数
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time);              // 接收到数据时的事件

    TcpServer _server;
    EventLoop *_loop;
};

#endif