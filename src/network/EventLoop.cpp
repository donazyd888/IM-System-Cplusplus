#include "network/EventLoop.h"
#include <vector>
#include <sys/epoll.h>

EventLoop::EventLoop() : epoll_(std::make_unique<Epoll>()), quit_(false) {
}

EventLoop::~EventLoop() {
    // std::unique_ptr 会自动清理 epoll_ 资源，无需手动 delete
}

void EventLoop::loop() {
    while (!quit_) {
        // 1. 调用底层的 epoll_wait 阻塞等待事件发生
        // 这里不传参数，默认 -1，表示如果没有事件发生，所在线程就一直休眠，不消耗 CPU
        std::vector<struct epoll_event> active_events = epoll_->poll();

        // 2. 遍历所有发生的活跃事件
        for (auto& ev : active_events) {
            int fd = ev.data.fd;
            uint32_t revents = ev.events;

            // 3. 从映射表中找到该 fd 对应的 Channel
            auto it = channels_.find(fd);
            if (it != channels_.end()) {
                Channel* channel = it->second;

                // 4. 将实际发生的事件类型（读/写/断开）设置给 Channel
                channel->setRevents(revents);

                // 5. 触发 Channel 的回调函数（去执行你写的具体业务逻辑）
                channel->handleEvent();
            }
        }
    }
}

void EventLoop::updateChannel(Channel* channel) {
    // 将传入的 Channel 按照其 fd 登记到哈希表中
    int fd = channel->getFd();
    channels_[fd] = channel;
}

Epoll* EventLoop::getEpoll() const {
    return epoll_.get();
}