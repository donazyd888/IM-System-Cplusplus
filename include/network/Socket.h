#pragma once
#include <sys/socket.h> // Linux 网络编程核心：提供 socket(), bind(), listen() 等函数
#include <arpa/inet.h>  // 提供 IP 地址转换工具
#include <unistd.h>     // 提供 close() 函数，用来关闭文件描述符
#include <fcntl.h>      // 提供 fcntl() 函数，用来设置非阻塞模式
#include <stdexcept>    // 异常处理机制
#include <string>       // C++ 字符串类

class Socket {
private:
    int fd_; // 核心资源：文件描述符

public:
    // 默认构造函数：创建一个新的 TCP Socket
    Socket();

    // 带参构造函数：用已有的 fd 接管 Socket（常用于 accept 返回的客户端连接）
    explicit Socket(int fd);

    // 析构函数：负责彻底释放 fd 资源
    ~Socket();

    // 【核心规范】禁用拷贝构造和拷贝赋值
    // 如果不禁用，两个 Socket 对象内部会保存相同的 fd，析构时会导致同一个 fd 被 close 两次，引发崩溃。
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    // 允许移动语义：转移 fd 的所有权
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;

    // 核心网络 API 的面向对象封装
    void bind(const std::string& ip, uint16_t port);
    void listen();
    int accept(); // 返回新接收到的客户端 fd

    // 配合 Reactor 模式的核心：设置为非阻塞模式
    void setNonBlocking();

    // 获取底层 fd
    int getFd() const;
};
