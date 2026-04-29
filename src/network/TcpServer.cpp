#include "network/TcpServer.h"
#include <iostream>

TcpServer::TcpServer(EventLoop* loop, const std::string& ip, uint16_t port)
    : loop_(loop),
      acceptor_(std::make_unique<Acceptor>(loop, ip, port)) {
    
    // 当 Acceptor 监听到新连接时，回调 TcpServer 的 newConnection 函数
    acceptor_->setNewConnectionCallback([this](int sockfd) {
        this->newConnection(sockfd);
    });
}

TcpServer::~TcpServer() {
    std::cout << "[TcpServer] Destroying server and all connections..." << std::endl;
}

void TcpServer::start() {
    std::cout << "[TcpServer] Server started." << std::endl;
    // (如果后续扩展为多线程版本，这里会启动线程池，目前单线程版本只需打印日志即可)
}

void TcpServer::newConnection(int sockfd) {
    // 1. 为新来的 sockfd 创建一个专属的 TcpConnection 对象
    // 使用 std::make_shared 进行内存分配，保证安全的生命周期管理
    auto conn = std::make_shared<TcpConnection>(loop_, sockfd);

    // 2. 将存在 TcpServer 里的业务回调，逐级传递给新创建的 TcpConnection
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    
    // 3. 设置连接断开时的内部清理回调 (负责把 conn 从 connections_ 映射表中删掉)
    conn->setCloseCallback([this](std::shared_ptr<TcpConnection> c) {
        this->removeConnection(c);
    });

    // 4. 将新连接加入存活名单
    connections_[sockfd] = conn;

    // 5. 通知连接已建立，开始监听读事件并触发上层的 ConnectionCallback
    conn->connectEstablished();
}

void TcpServer::removeConnection(std::shared_ptr<TcpConnection> conn) {
    // 从映射表中移除该连接。
    // 一旦移除，如果业务层没有额外持有它的 shared_ptr，该连接的引用计数将归零，
    // 随即自动触发 TcpConnection 的析构函数，彻底释放 Socket 和 Channel 资源。
    connections_.erase(conn->getFd());
}