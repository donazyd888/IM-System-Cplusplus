#pragma once
#include <functional>
#include <memory>
#include <string>
#include "network/EventLoop.h"
#include "network/Socket.h"
#include "network/Channel.h"

class Acceptor {
private:
    EventLoop* loop_;                                // 归属的事件循环
    std::unique_ptr<Socket> listen_socket_;          // 专门用于监听的服务端 Socket
    std::unique_ptr<Channel> listen_channel_;        // 绑定监听 Socket 的 Channel
    std::function<void(int)> new_connection_callback_; // 获取到新连接后的回调函数

    // 内部处理新连接的逻辑
    void acceptConnection();

public:
    Acceptor(EventLoop* loop, const std::string& ip, uint16_t port);
    ~Acceptor();

    // 禁用拷贝
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    // 设置上层业务处理新连接的回调
    void setNewConnectionCallback(std::function<void(int)> cb);
};