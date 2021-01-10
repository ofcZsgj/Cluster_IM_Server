#include "db_connection.hpp"

using namespace std;

// 初始化数据库连接
Connection::Connection()
{
    _conn = mysql_init(nullptr);
}

// 释放数据库连接资源
Connection::~Connection()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}

// 连接数据库
bool Connection::connect(string ip, unsigned short port, string user, string password, string dbname)
{
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0);
    if (p != nullptr)
    {
        // C和C++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文显示？
        mysql_query(_conn, "set names gbk");
        LOG_INFO << "connect mysql success!";
    }
    else
    {
        LOG_INFO << "connect mysql fail!";
    }

    return p != nullptr;
}

// 更新操作 insert delete update
bool Connection::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "QUERY FALSE!";
        return false;
    }
    return true;
}

// 查询操作 select
MYSQL_RES *Connection::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "UPDATE FALSE!";
        return nullptr;
    }
    return mysql_use_result(_conn);
}

// 获取与MySQL的一条连接
MYSQL *Connection::getMySQLConnection()
{
    return _conn;
}