#include "chat_server.hpp"

#include <iostream>
using namespace std;

int main()
{
    EventLoop loop;                      // epoll create
    InetAddress addr("127.0.0.1", 6000); // ip:port
    ChatServer server(&loop, addr, "ChatServer");

    server.start(); // listenfd epoll_ctl -> epoll
    loop.loop();    // epoll_waitSS

    return 0;
}