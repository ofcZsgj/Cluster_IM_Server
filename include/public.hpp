#ifndef PUBLIC_H
#define PUBLIC_H

/**
 * server和client的公共文件
 */

// 消息类型
enum EnMsgType
{
    LOGIN_MSG = 1,    // 1 登陆消息
    LOGIN_MSG_ACK,    // 2 登陆响应消息
    REG_MSG,          // 3 注册消息
    REG_MSG_ACK,      // 4 注册响应消息
    ONE_CHAT_MSG,     // 5 一对一聊天消息
    ADD_FRIEND_MSG,   // 6 添加好友信息
    CREATE_GROUP_MSG, // 7 创建群组信息
    ADD_GROUP_MSG,    // 8 加入群组信息
    GROUP_CHAT_MSG,   // 9 群聊消息
};

#endif