#include "chat_server.hpp"
#include "chat_service.hpp"

#include <signal.h>
#include <iostream>
using namespace std;

void resetHandle(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main()
{
    signal(SIGINT, resetHandle);

    EventLoop loop;                      // epoll create
    InetAddress addr("127.0.0.1", 6000); // ip:port
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl -> epoll
    loop.loop();    // epoll_wait

    return 0;
}