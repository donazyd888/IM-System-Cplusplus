#pragma once
#include <memory>
#include <unordered_map>
#include "network/Epoll.h"
#include "network/Channel.h"

class EventLoop {
private:
    std::unique_ptr<Epoll> epoll_;                    // EventLoop 拥有并管理底层的 Epoll 实例
    bool quit_;                                       // 控制事件循环退出的标志位
    std::unordered_map<int, Channel*> channels_;      // 核心映射：fd -> Channel*

public:
    EventLoop();
    ~EventLoop();

    // 禁用拷贝
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    // 开启事件循环（服务器的心脏，这是一个阻塞函数，直到 quit_ 变为 true）
    void loop();

    // 将 Channel 注册到 EventLoop 的管理簿中
    void updateChannel(Channel* channel);

    // 暴露底层的 Epoll 指针，供后续创建 Channel 时使用
    Epoll* getEpoll() const;
};