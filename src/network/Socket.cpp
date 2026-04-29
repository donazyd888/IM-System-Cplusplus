#include "network/Socket.h"
#include <iostream>
#include <sys/socket.h> // 确保包含 setsockopt 需要的头文件

Socket::Socket() {
    // AF_INET 表示 IPv4，SOCK_STREAM 表示 TCP 协议
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ == -1) {
        throw std::runtime_error("Failed to create socket!");
    }
}

Socket::Socket(int fd) : fd_(fd) {
}

Socket::~Socket() {
    if (fd_ != -1) {
        ::close(fd_);
        // 在实际开发中，这里可以接入之前提到的 spdlog 打印资源释放日志
    }
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1; // 剥夺原对象的 fd 所有权，防止其在析构时被关闭
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        if (fd_ != -1) {
            ::close(fd_); // 释放当前可能持有的资源
        }
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

void Socket::bind(const std::string& ip, uint16_t port) {
    // 🌟 核心修复：开启 SO_REUSEADDR 端口复用
    // 这样即使 WSL 界面没关或者有残留进程导致端口处于 TIME_WAIT 状态，也能强行绑定成功
    int optval = 1;
    if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval)) == -1) {
        std::cerr << "Warning: Failed to set SO_REUSEADDR" << std::endl;
    }

    // 可选：开启 SO_REUSEPORT (现代 Linux 系统都支持，进一步增强端口复用能力)
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));


    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); // 主机字节序转网络字节序 (大端)

    // 将字符串 IP 转换为网络字节序的整数
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid IP address!");
    }

    if (::bind(fd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        throw std::runtime_error("Failed to bind socket!");
    }
}

void Socket::listen() {
    // 128 是 SOMAXCONN 的常见默认值，定义了操作系统底层连接队列的长度
    if (::listen(fd_, 128) == -1) {
        throw std::runtime_error("Failed to listen on socket!");
    }
}

int Socket::accept() {
    struct sockaddr_in client_addr{};
    socklen_t client_addr_len = sizeof(client_addr);

    // 接收连接
    int client_fd = ::accept(fd_, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        // 在非阻塞模式下，EAGAIN/EWOULDBLOCK 不是致命错误，后续会处理
        throw std::runtime_error("Failed to accept new connection!");
    }
    return client_fd;
}

void Socket::setNonBlocking() {
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        throw std::runtime_error("fcntl F_GETFL failed!");
    }
    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::runtime_error("fcntl F_SETFL O_NONBLOCK failed!");
    }
}

int Socket::getFd() const {
    return fd_;
}