#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "db_connection.hpp"

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>

using namespace std;

// 实现连接池功能模块
class ConnectionPool
{
public:
    // #2: 提供获取连接池对象的实例
    static ConnectionPool *getConnectionPool();

    // 给外部提供接口，从连接池中获取一个空闲 可用的连接
    shared_ptr<Connection> getConnection();

private:
    // 单例模式 #1: 构造函数私有化
    ConnectionPool();

    // 从配置文件中加载配置项
    bool loadConfigFile();

    // 运行在独立的线程中，专门负责生产新连接（线程函数作为成员方法方便访问）
    void produceConnectionTask();

    // 扫描超过maxIdleTime时间的空闲连接并进行回收
    void scannerConnectionTask();

    string _ip;             // mysql的ip地址
    unsigned int _port;     // mysql的端口号
    string _username;       // mysql登陆用户名
    string _password;       // mysql登陆密码
    string _dbName;         // mysql数据库名
    int _initSize;          // 连接池的初始连接量
    int _maxSize;           // 连接池的最大连接量
    int _maxIdleTime;       // 连接池的最大空闲时间
    int _connectionTimeout; // 连接池获取连接的超时时间

    queue<Connection *> _connectionQue; // 存储mysql连接的队列
    mutex _queueMutex;                  // 维护连接队列的线程安全互斥锁
    atomic_int _connectionCnt;          // 记录连接所创建的connection连接总数量
    condition_variable cv;              // 设置条件变量用于连接生产线程和连接消费线程的通信
};

#endif