#include "network/Channel.h"
#include "network/Epoll.h"
#include <sys/epoll.h>

Channel::Channel(Epoll* epoll, int fd) 
    : epoll_(epoll), fd_(fd), events_(0), revents_(0), in_epoll_(false) {
}

Channel::~Channel() {
    // 析构时不需要关闭 fd，因为 fd 的生命周期由 Socket 类管理，Channel 只做“借用”
}

void Channel::enableReading() {
    events_ |= EPOLLIN | EPOLLPRI; // 监听可读和高优先级带外数据可读
    if (!in_epoll_) {
        epoll_->addFd(fd_, events_);
        in_epoll_ = true;
    } else {
        epoll_->modFd(fd_, events_);
    }
}

void Channel::handleEvent() {
    // EPOLLIN: 有数据可读
    // EPOLLPRI: 有紧急数据可读
    // EPOLLRDHUP: 对端关闭连接或关闭了写操作一半
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (read_callback_) {
            read_callback_();
        }
    }
    
    // EPOLLOUT: 缓冲区可写
    if (revents_ & EPOLLOUT) {
        if (write_callback_) {
            write_callback_();
        }
    }
}

void Channel::setRevents(uint32_t rev) {
    revents_ = rev;
}

void Channel::setReadCallback(std::function<void()> cb) {
    read_callback_ = std::move(cb);
}

void Channel::setWriteCallback(std::function<void()> cb) {
    write_callback_ = std::move(cb);
}

int Channel::getFd() const {
    return fd_;
}