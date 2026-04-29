# High-Concurrency IM System (Based on Reactor Pattern)
### 基于 Reactor 模式的高并发即时通讯系统

本项目是一个轻量级、高性能的即时通讯系统，旨在解决高并发场景下的网络 I/O 瓶颈。服务端采用 C++11 编写，基于 Reactor 事件驱动模型和 Epoll 多路复用技术实现；客户端基于 Electron 跨平台框架构建。 

## 🌟 核心特性 (Key Features)
* **高并发网络引擎**：基于 Linux Epoll 实现的 Reactor 模型，支持万级并发连接。 
* **非阻塞 I/O 设计**：自主封装应用层 Buffer 缓冲区，彻底解决 TCP 粘包与半包问题。 
* **实时通讯架构**：支持低延迟的点对点（P2P）文本消息转发。 
* **离线消息管理**：通过 MySQL 实现离线消息的云端暂存，并在用户上线时自动漫游推送。 

## 🏗️ 系统架构 (Architecture)
* **Main Reactor**: 负责监听 `listen_fd`，处理新用户的连接请求。 
* **Event Loop**: 核心事件循环，负责分发 `read/write` 事件。 
* **Data Persistence**: MySQL 8.0 负责存储用户信息、好友关系及离线报文。 
* **Protocol**: 采用轻量级 JSON 格式作为应用层通信协议。 

## 🚀 快速开始 (Getting Started)

### 1. 服务端编译 (Server Build)
**环境要求：CMake 3.10+, GCC 7.5+, MySQL 8.0**

```bash
mkdir build && cd build
cmake ..
make
./IMServer
```

### 2.客户端运行 (Client Run)
```bash
cd client
npm install
npm start
```

## 客户端登入界面：
<img width="208" height="264" alt="image" src="https://github.com/user-attachments/assets/7978764e-7a38-48aa-82d5-3ce06f70fc68" />

## 客户端聊天界面：
<img width="246" height="348" alt="image" src="https://github.com/user-attachments/assets/f850b391-9c2b-4256-8dd4-c02bf4061ee3" />

## 总体结构设计：
<img width="439" height="213" alt="image" src="https://github.com/user-attachments/assets/4b5a6043-ee65-4c56-afc7-ba2f12c195b5" />

## 数据流图：
<img width="431" height="238" alt="image" src="https://github.com/user-attachments/assets/a736efd2-9397-4d30-8c6d-0e9f0a910f0e" />

## 系统核心ER图：
<img width="457" height="216" alt="image" src="https://github.com/user-attachments/assets/5f94e68b-d00c-4355-89c9-037ff5242061" />




