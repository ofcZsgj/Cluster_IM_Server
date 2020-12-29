#include "group_module.hpp"
#include "mysql.hpp"

// 创建群组
bool GroupModule::createGroup(Group &group)
{
    char sql[128] = {0};
    sprintf(sql, "insert into all_group(group_name, desc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的群组数据生成的主键自增ID
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
