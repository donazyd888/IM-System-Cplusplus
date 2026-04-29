#pragma once
#include <memory>
#include <string>
#include <functional>
#include "network/EventLoop.h"
#include "network/Socket.h"
#include "network/Channel.h"
#include "utils/Buffer.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    // 回调函数类型定义
    // std::shared_ptr<TcpConnection> 用于保证在回调执行期间，连接对象不会被销毁
    using ConnectionCallback = std::function<void(std::shared_ptr<TcpConnection>)>;
    using MessageCallback = std::function<void(std::shared_ptr<TcpConnection>, Buffer*)>;
    using CloseCallback = std::function<void(std::shared_ptr<TcpConnection>)>;

    TcpConnection(EventLoop* loop, int fd);
    ~TcpConnection();

    // 禁用拷贝
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    // 核心操作
    void send(const std::string& message); // 发送消息
    void connectEstablished();             // 连接建立时调用
    void connectDestroyed();               // 连接销毁时调用

    // 设置回调
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }

    int getFd() const { return socket_->getFd(); }

    bool connected() const { return isConnected_; }

private:
    EventLoop* loop_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    Buffer inputBuffer_;  // 接收缓冲区
    Buffer outputBuffer_; // 发送缓冲区

    // 业务层传入的回调
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    CloseCallback closeCallback_;

    // 内部事件处理函数，绑定给 Channel
    void handleRead();
    void handleWrite();
    void handleClose();

    bool isConnected_ = false;
};