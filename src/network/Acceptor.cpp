#include "network/Acceptor.h"

Acceptor::Acceptor(EventLoop* loop, const std::string& ip, uint16_t port)
    : loop_(loop),
      listen_socket_(std::make_unique<Socket>()),
      listen_channel_(std::make_unique<Channel>(loop->getEpoll(), listen_socket_->getFd())) {
    
    // 1. 设置非阻塞并绑定端口监听
    listen_socket_->setNonBlocking();
    listen_socket_->bind(ip, port);
    listen_socket_->listen();

    // 2. 将“接收新连接”这个动作，注册为监听 Channel 的可读事件回调
    listen_channel_->setReadCallback([this]() {
        this->acceptConnection();
    });

    // 3. 开启对新连接的监听，并加入到 EventLoop 中
    listen_channel_->enableReading();
    loop_->updateChannel(listen_channel_.get());
}

Acceptor::~Acceptor() {
}

void Acceptor::setNewConnectionCallback(std::function<void(int)> cb) {
    new_connection_callback_ = std::move(cb);
}

void Acceptor::acceptConnection() {
    // 接受底层的连接，拿到客户端的 fd
    int client_fd = listen_socket_->accept();
    
    // 如果上层设置了回调，就把这个新 fd 丢给上层处理
    if (new_connection_callback_) {
        new_connection_callback_(client_fd);
    }
}