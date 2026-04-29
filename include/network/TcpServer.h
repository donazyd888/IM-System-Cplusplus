#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include "network/EventLoop.h"
#include "network/Acceptor.h"
#include "network/TcpConnection.h"

class TcpServer {
public:
    // 将 TcpConnection 中定义的回调类型“借”过来，方便上层使用
    using ConnectionCallback = TcpConnection::ConnectionCallback;
    using MessageCallback = TcpConnection::MessageCallback;

    TcpServer(EventLoop* loop, const std::string& ip, uint16_t port);
    ~TcpServer();

    // 禁用拷贝
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    // 暴露给业务层的回调注册接口
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }

    // 启动服务器
    void start();

private:
    EventLoop* loop_;
    std::unique_ptr<Acceptor> acceptor_;
    
    // 核心映射：维护所有当前存活的客户端连接 (fd -> TcpConnection)
    std::unordered_map<int, std::shared_ptr<TcpConnection>> connections_;

    // 业务层传入的回调
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;

    // 内部处理新连接和连接断开的逻辑
    void newConnection(int sockfd);
    void removeConnection(std::shared_ptr<TcpConnection> conn);
};