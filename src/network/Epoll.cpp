//
// Created by Admin on 2026/4/4.
//
#include "network/Epoll.h"
#include <string.h>

Epoll::Epoll() : epfd_(-1), events_(1024) { // 默认初始化能容纳 1024 个并发事件的数组
    // epoll_create1(0) 是比老版本 epoll_create 更推荐的用法
    epfd_ = ::epoll_create1(0);
    if (epfd_ == -1) {
        throw std::runtime_error("Failed to create epoll instance!");
    }
}

Epoll::~Epoll() {
    if (epfd_ != -1) {
        ::close(epfd_);
    }
}

void Epoll::addFd(int fd, uint32_t op) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = op;

    if (::epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        throw std::runtime_error("epoll_ctl ADD failed!");
    }
}

void Epoll::modFd(int fd, uint32_t op) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = op;

    if (::epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
        throw std::runtime_error("epoll_ctl MOD failed!");
    }
}

void Epoll::delFd(int fd) {
    // 移除时不需要传 event 结构体
    if (::epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        throw std::runtime_error("epoll_ctl DEL failed!");
    }
}

std::vector<struct epoll_event> Epoll::poll(int timeout_ms) {
    std::vector<struct epoll_event> active_events;

    // epoll_wait 会将发生状态变化的事件填充到 events_.data() 指向的内存中
    int num_events = ::epoll_wait(epfd_, events_.data(), events_.size(), timeout_ms);

    if (num_events == -1) {
        throw std::runtime_error("epoll_wait failed!");
    }

    // 将收集到的活跃事件放入返回的 vector 中
    for (int i = 0; i < num_events; ++i) {
        active_events.push_back(events_[i]);
    }

    return active_events;
}