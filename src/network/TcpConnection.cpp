//
// Created by Admin on 2026/4/6.
//
#include "network/TcpConnection.h"
#include <iostream>
#include <unistd.h>

TcpConnection::TcpConnection(EventLoop* loop, int fd)
    : loop_(loop),
      socket_(std::make_unique<Socket>(fd)),
      channel_(std::make_unique<Channel>(loop->getEpoll(), fd)) {

    // 设置非阻塞模式
    socket_->setNonBlocking();

    // 给 Channel 绑定当前类的处理函数
    channel_->setReadCallback([this]() { handleRead(); });
    channel_->setWriteCallback([this]() { handleWrite(); });
}

TcpConnection::~TcpConnection() {
    std::cout << "[TcpConnection] Destroyed for FD " << socket_->getFd() << std::endl;
}

void TcpConnection::connectEstablished() {
    // 🌟 修改点 1：标记状态为已连接
    isConnected_ = true;

    // 开启读事件监听，注册到 EventLoop 中
    channel_->enableReading();
    loop_->updateChannel(channel_.get());

    // 触发上层业务的连接回调 (此时 main.cpp 里的 connected() 会返回 true)
    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectDestroyed() {
    // 将连接从 epoll 中彻底移除 (这里简单处理)
    if (closeCallback_) {
        // 真正的资源释放由 shared_ptr 计数归零触发
    }
}

void TcpConnection::handleRead() {
    int savedErrno = 0;
    // 把底层 Socket 的数据直接读到应用层的 inputBuffer_ 里！
    ssize_t n = inputBuffer_.readFd(socket_->getFd(), &savedErrno);

    if (n > 0) {
        // 成功读取到数据，触发业务层的消息回调
        if (messageCallback_) {
            messageCallback_(shared_from_this(), &inputBuffer_);
        }
    } else if (n == 0) {
        // read 返回 0，代表对端关闭了连接
        handleClose();
    } else {
        // 出错了
        std::cerr << "TcpConnection::handleRead failed, fd = " << socket_->getFd() << std::endl;
        handleClose();
    }
}

void TcpConnection::send(const std::string& message) {
    outputBuffer_.append(message);

    // 尝试立刻写出
    ssize_t nwrote = 0;
    size_t remaining = outputBuffer_.readableBytes();

    nwrote = ::write(socket_->getFd(), outputBuffer_.peek(), remaining);
    if (nwrote >= 0) {
        remaining -= nwrote;
        outputBuffer_.retrieve(nwrote); // 移动缓冲区指针
    }

    if (remaining > 0) {
        // TODO: 开启写事件监听，在 handleWrite 中继续发送剩余数据
    }
}

void TcpConnection::handleWrite() {
    // 处理 EPOLLOUT 事件，继续发送 outputBuffer_ 中的数据
}

void TcpConnection::handleClose() {
    // 🌟 修改点 2：第一时间标记状态为已断开
    isConnected_ = false;

    std::cout << "[TcpConnection] Closed for FD " << socket_->getFd() << std::endl;

    // 🌟 修改点 3：触发上层业务的连接状态回调，通知 main.cpp 执行 else 分支的哈希表清理逻辑
    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }

    // 触发内部 TcpServer 的清理回调
    if (closeCallback_) {
        closeCallback_(shared_from_this());
    }
}