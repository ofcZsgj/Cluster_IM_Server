#include "user.hpp"
#include "group.hpp"
#include "public.hpp"
#include "json.hpp"

#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <unordered_map>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
using json = nlohmann::json;

// 记录当前系统登陆的用户信息
User g_currentUser;
// 记录当前登陆用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前的登陆用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 接收线程
void readTaskHandler(int clientfd);

// 聊天客户端程序实现 main线程用作发送线程，子线程用于接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的TCP socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server建立连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // main线程用户接收用户输入，负责发送数据
    while (1)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "========================" << endl;
        cout << "choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读取掉缓冲区中的回车

        switch (choice)
        {
        case 1: // login业务
        {
            int id = 0;
            char pwd[20] = {0};
            cout << "userid:";
            cin >> id;
            cin.get(); // 读取掉缓冲区中的回车
            cout << "userpassword:";
            cin.getline(pwd, 20);

            // 对登陆消息，用户输入的id和密码进行json序列化并发送给服务器
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str() + 1), 0);

            if (len == -1)
            {
                cerr << "send login msg error:" << request << endl;
            }
            else
            {
                // 接收服务端传回的消息，并进行判断解析
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 登陆失败
                    {
                        cerr << responsejs["errmssg"] << endl;
                    }
                    else // 登陆成功
                    {
                        //记录当前用户的id和name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录当前用户的好友列表信息
                        if (responsejs.contains("friends"))
                        {
                            // 初始化
                            g_currentUserFriendList.clear();

                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        // 记录当前用户的群组列表信息
                        if (responsejs.contains("groups"))
                        {
                            // 初始化
                            g_currentUserGroupList.clear();

                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupstr : vec1)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                vector<string> vec2 = grpjs["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json userjs = json::parse(userstr);
                                    user.setId(userjs["id"].get<int>());
                                    user.setName(userjs["name"]);
                                    user.setState(userjs["state"]);
                                    user.setRole(userjs["role"]);
                                    group.getUsers().push_back(user);
                                }

                                g_currentUserGroupList.push_back(group);
                            }
                        }
                    }
                }
            }
        }
        break;
        case 2: // register业务
        {
            // 接收用户输入的用户名和密码
            char name[12] = {0};
            char pwd[20] = {0};
            cout << "username:";
            cin.getline(name, 12);
            cout << "userpassword:";
            cin.getline(pwd, 20);

            // 将注册消息，用户名，密码进行json序列化后发送到服务器
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();
            int len = send(clientfd, request.c_str(), strlen(request.c_str() + 1), 0);

            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            // 接收并解析服务器传回来的消息
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (len == -1)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) //注册失败
                    {
                        cerr << name << " is already exist, register error!" << endl;
                    }
                    else // 注册成功
                    {
                        cout << name << "register success, your userid is " << responsejs["id"] << ", do not forget it!" << endl;
                    }
                }
            }
        }
        break;
        case 3: // quit业务
        {
            close(clientfd);
            exit(0);
        }
        break;
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }

    return 0;
}