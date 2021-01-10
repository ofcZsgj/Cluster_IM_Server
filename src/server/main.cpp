#include "chat_server.hpp"
#include "chat_service.hpp"
#include "connection_pool.hpp"

#include <signal.h>
#include <iostream>
using namespace std;

void resetHandle(int)
{
    ChatService::instance()->reset();
    exit(0);
}

// 获取数据库连接池对象的唯一实例
ConnectionPool *dbpool = ConnectionPool::getConnectionPool();

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    signal(SIGINT, resetHandle);

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    EventLoop loop;             // epoll create
    InetAddress addr(ip, port); // ip:port
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl -> epoll
    loop.loop();    // epoll_wait

    return 0;
}