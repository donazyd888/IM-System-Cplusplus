#pragma once
#include <functional>
#include <cstdint>

// 前向声明，避免交叉包含
class Epoll; 

class Channel {
private:
    Epoll* epoll_;      // 该 Channel 归属的 Epoll 实例
    int fd_;            // 绑定的文件描述符
    uint32_t events_;   // 期望 epoll 监听的事件 (如 EPOLLIN, EPOLLOUT)
    uint32_t revents_;  // epoll 实际返回的已发生事件
    bool in_epoll_;     // 标记当前 Channel 的 fd 是否已经添加到了 epoll 树上

    // 回调函数：当事件发生时，要执行的业务代码
    std::function<void()> read_callback_;
    std::function<void()> write_callback_;

public:
    Channel(Epoll* epoll, int fd);
    ~Channel();

    // 禁用拷贝
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    // 核心逻辑：根据 revents_ 的状态，将事件路由到对应的回调函数
    void handleEvent(); 

    // 开启对读事件的监听，并将其注册或更新到 epoll 中
    void enableReading();

    // Setter 方法
    void setRevents(uint32_t rev);
    void setReadCallback(std::function<void()> cb);
    void setWriteCallback(std::function<void()> cb);

    // Getter 方法
    int getFd() const;
};