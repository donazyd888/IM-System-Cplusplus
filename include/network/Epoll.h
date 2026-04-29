#pragma once
#include <sys/epoll.h>
#include <vector>
#include <stdexcept>
#include <unistd.h>

class Epoll {
private:
    int epfd_;                                // epoll 实例的文件描述符
    std::vector<struct epoll_event> events_;  // 存放 epoll_wait 返回的活跃事件

public:
    Epoll();
    ~Epoll();

    // 同样禁用拷贝构造和赋值操作
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    // 将 Socket 的 fd 注册到 epoll 中，并监听指定的事件（如可读、可写）
    void addFd(int fd, uint32_t op);
    
    // 修改已经注册的 fd 的监听事件
    void modFd(int fd, uint32_t op);
    
    // 将 fd 从 epoll 中移除
    void delFd(int fd);

    // 核心函数：阻塞等待，直到有事件发生，返回发生事件的列表
    // timeout_ms = -1 表示永久阻塞，0 表示立即返回，>0 表示最多阻塞指定毫秒数
    std::vector<struct epoll_event> poll(int timeout_ms = -1);
};