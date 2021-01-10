# A C++ High Performance Cluster IM Server 

## Introduction  
本项目为C++11编写的集群聊天服务器，使用CMake进行编译，实现了客户端的注册，登陆、登出，添加好友、群组，私聊，群聊，离线消息，好友列表，群组列表等功能，支持客户端跨服务器通信。

## Envoirment  
* OS: Ubuntu 16.04.7 LTS
* Complier: g++ 5.4
* Cmake: 4.1
* Boost: 1.69
* Muduo: 2.0.1
* MySQL: 5.7.31	
* Redis: 3.0.6
* Hiredis: 1.0.0
* Nginx: 1.12.2

## Build
./build.sh

## Technical points
* 聊天服务器基于muduo网络库开发，通过muduo的消息回调处理各类业务，连接回调对异常退出的客户端进行状态重置，利用muduo的日志系统在服务器端打印关键信息
* 基于发布-订阅的Redis消息队列实现跨服务器通信。服务器单独开启一个线程阻塞监听订阅通道上的事件并通过回调给业务层上报，用户订阅通道后则无需阻塞等待接收通道消息
* 配置Nginx实现TCP负载均衡
* 模块分层
	* 业务类注册好各类消息的事件回调，并将消息种类与对应回调存储在unordered_map的成员中，当网络模块的muduo消息回调监测到事件发生时，先反序列化消息获得消息类型后，再通过业务类的单例在unordered_map成员中找到匹配的回调方法进行调用，达到网络模块与业务模块的解耦
	* 通过定义ORM类与各个数据表操作的接口方法达到业务模块与数据模块的解耦 
* 实现数据库连接池
	* 经过测试在数据量为10000时，单线程下使用连接池较不使用提高83%,4线程时提高了26%
	* 连接池为线程安全的懒汉单例模式
	* 空闲的连接均维护在一个的线程安全的队列中，使用基于CAS的原子整型进行计数
	* 生产者线程与消费者线程使用条件变量和unique_lock实现了线程间通信及保证线程安全
	* 用户获取到的连接使用shared_ptr智能指针来管理，用lambada表达式定制连接释放的功能
	* 回收线程定期扫描整个队列释放多余的连接
* MySQL存储用户信息，群组信息，好友信息，关系信息，离线消息
* 使用Json For Morden C++实现数据的序列化与反序列化

## 代码结构
<img src="/Users/zsgj/Library/Mobile Documents/com~apple~CloudDocs/Markdownf Note/images/20210110144032.png"  />

## 代码统计
<img src="/Users/zsgj/Library/Mobile Documents/com~apple~CloudDocs/Markdownf Note/images/20210110143604.png"  />

## 开发过程中所遭遇的问题
1. stack smashing detected
   - 操作MySQL时，定义存放sql语句的数组空间过小，在存储离线群聊消息时，由于参数有要存储的离线消息，因此长度超过定义的128字节导致服务器异常退出。
   - 解决办法：增大存放sql语句的数组空间大小
2. 两个客户端分别登陆在两台服务器上，在使用hiredis提供的发送redis命令行接口函数redisCommand，在发送发布或者订阅命令时无效
   - 解决办法：在发送发布-订阅命令时，需要在不同的上下文Context环境中进行，不能够在同一Context下进行。
3. 在引入redis发布-订阅后客户端登陆以后没有响应
   - ps -ef | grep Server获得服务器的pid
   - sudo gdb Server.out
   - 通过gdb调试服务器进程：attach 5908
   - 打印当前进程所有线程信息：info threads得到六条信息，这是由于我在构造muduo的TcpServer时设置了服务端的线程数量为N=6，其中Server.out为主线程，即muduo库的I/O线程(muduo会设置1个I/O线程与N-1个worker线程)，EventLoop事件循环有5线程，分别是ChatServer0、ChatServer1 ...  ChatServer5，其中ChatServer1至ChatServer5均处在epoll_wait状态，等待已连接用户的读写事件，但是ChatServer0却阻塞在__libc_recv函数处，不能继续处理逻辑业务，不能给客户端回复响应，导致客户端无应答。
   - 调试ChatServer0：thread 2
   - 从ChatServer0线程的调用堆栈打印信息（bt命令）看到这个thread 2在处理客户端的登录请求时，需要向redis中间件消息队列订阅消息，是通过hiredis的redisCommand发送的subscribe订阅命令，但是通过调用堆栈信息查看，redisCommand不仅可以发送subscribe订阅命令到redis server，还会以阻塞的方式等待redis server的响应。但实际上，项目代码上已经开启了一个独立的线程，专门做redisGetReply，由于线程池里面的redisGetReply抢了上面订阅subscribe的redisCommand底层调用的redisGetReply的响应消息，导致ChatServer0线程阻塞在这个接口调用上，无法再次回到epoll_wait处了，这个线程就废掉了，如果工作线程全部发生这种情况，最终服务器所有的工作线程就全部停止工作了
   - 解决办法：
     - 从hiredis的redisCommand源码上可以看出，它实际上相当于调用了这三个函数：
     - redisAppendCommand 把命令写入本地发送缓冲区
     - redisBufferWrite 把本地缓冲区的命令通过网络发送出去
     - redisGetReply 阻塞等待redis server响应消息
     - 因为已经在muduo库的ThreadPool中单独开辟了一个线程池，接收this->_context上下文的响应消息，那么subcribe订阅消息只做消息发送，不做消息接收就可以了。即在订阅时不使用redisAppendCommand方法，而是使用redisAppendCommand和redisBufferWrite，避免调用到redisGetReply引起阻塞。由此问题解决