#include "connection_pool.hpp"

// 线程安全对懒汉单例函数接口
ConnectionPool *ConnectionPool::getConnectionPool()
{
    // 对于static对象对初始化，编译器会自动提供lock 和 unlock 线程安全
    static ConnectionPool pool;
    return &pool;
}

// 从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
    FILE *pf = fopen("../dbpool.cnf", "r"); // 这里的相对路径是相对于生成的可执行文件而言
    if (pf == nullptr)
    {
        return false;
    }

    while (!feof(pf))
    {
        char line[1024] = {0};
        fgets(line, 1024, pf);
        string str = line;
        int idx = str.find('=', 0);
        if (idx == -1) // 无效配置项
        {
            continue;
        }
        // username=root\n
        int endidx = str.find('\n', idx);
        string key = str.substr(0, idx);
        string value = str.substr(idx + 1, endidx - idx - 1);

        if (key == "ip")
        {
            _ip = value;
        }
        else if (key == "port")
        {
            _port = atoi(value.c_str());
        }
        else if (key == "username")
        {
            _username = value;
        }
        else if (key == "password")
        {
            _password = value;
        }
        else if (key == "dbName")
        {
            _dbName = value;
        }
        else if (key == "initSize")
        {
            _initSize = atoi(value.c_str());
        }
        else if (key == "maxSize")
        {
            _maxSize = atoi(value.c_str());
        }
        else if (key == "maxIdleTime")
        {
            _maxIdleTime = atoi(value.c_str());
        }
        else if (key == "connectionTimeout")
        {
            _connectionTimeout = atoi(value.c_str());
        }
    }
    return true;
}

// 连接池的构造
ConnectionPool::ConnectionPool()
{
    // 加载配置项
    if (!loadConfigFile())
    {
        return;
    }

    // 创建初始数量的连接
    for (int i = 0; i < _initSize; ++i)
    {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbName);
        p->refreshAliveTime();
        _connectionQue.push(p);
        ++_connectionCnt;
    }

    // 启动一个线程，作为连接的生产者 thread ->Linux pthread_create
    // 给这个成员方法绑定一个当前对象，否则无法作为一个线程函数（需要C接口）
    thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach();
    // 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接并进行回收
    thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
    scanner.detach();
}

// 运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
    for (;;)
    {
        unique_lock<mutex> lock(_queueMutex); //加锁
        while (!_connectionQue.empty())
        {
            // 队列不为空，生产者线程进入等待状态（释放锁）
            cv.wait(lock);
        }
        // 连接数量没有到达上限，继续创建新的连接
        if (_connectionCnt < _maxSize)
        {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbName);
            p->refreshAliveTime();
            _connectionQue.push(p);
            ++_connectionCnt;
        }

        // 通知消费者线程可以消费连接了
        cv.notify_all();
    }
}

// 给外部提供接口，从连接池中获取一个空闲 可用的连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(_queueMutex);
    while (_connectionQue.empty())
    {
        if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
        {
            if (_connectionQue.empty())
            {
                LOG_INFO << "get connection timeout";
                return nullptr;
            }
        }
    }

    // shared_ptr智能指针析构时，会把Connection资源delete掉。
    // 相当于调用Connection的析构函数，连接就被close掉了。
    // 这里需要自定义智能指针的释放资源方式，把Connection归还到queue中。
    shared_ptr<Connection> sp(_connectionQue.front(),
                              [&](Connection *pcon) {
                                  unique_lock<mutex> lock(_queueMutex);
                                  pcon->refreshAliveTime();
                                  _connectionQue.push(pcon);
                              });

    _connectionQue.pop();
    // 消费连接后，通知生产者检查队列是否为空
    cv.notify_all();

    return sp;
}

// 扫描超过maxIdleTime时间的空闲连接并进行回收
void ConnectionPool::scannerConnectionTask()
{
    for (;;)
    {
        // 通过sleep模拟定时效果
        this_thread::sleep_for(chrono::seconds(_maxIdleTime));

        // 扫描整个队列，释放多余的连接
        unique_lock<mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize)
        {
            Connection *p = _connectionQue.front();
            if (p->getAliveTime() >= (_maxIdleTime * 1000))
            {
                _connectionQue.pop();
                --_connectionCnt;
                // 调用～Connection()释放连接
                delete p;
            }
            else
            {
                // 队头元素没有超过_maxIdleTime，则其余连接肯定没有超过
                break;
            }
        }
    }
}