#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

class Buffer {
private:
    std::vector<char> buffer_; // 底层连续内存
    size_t readPos_;           // 读指针
    size_t writePos_;          // 写指针

public:
    // 默认初始化 1024 字节大小
    static const size_t kInitialSize = 1024; 

    explicit Buffer(size_t initialSize = kInitialSize);
    ~Buffer() = default;

    // 获取可读字节数 (writePos - readPos)
    size_t readableBytes() const;

    // 获取可写字节数 (buffer.size() - writePos)
    size_t writableBytes() const;

    // 获取前面空闲的字节数 (由于读取后 readPos 后移留下的空间)
    size_t prependableBytes() const;

    // 返回当前可读数据的首地址
    const char* peek() const;

    // 提取数据后，移动读指针
    void retrieve(size_t len);

    // 提取直到特定位置的数据（通常用于按协议包头包尾提取）
    void retrieveUntil(const char* end);

    // 清空缓冲区 (将读写指针复位)
    void retrieveAll();

    // 将可读数据转化为 std::string 返回，并移动指针
    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t len);

    // 追加数据到缓冲区
    void append(const char* data, size_t len);
    void append(const std::string& str);

    // 确保有足够的空间可写，如果不够则扩容或移动数据
    void ensureWritableBytes(size_t len);

    // 【核心亮点】从底层的 Socket fd 中高效读取数据
    ssize_t readFd(int fd, int* savedErrno);

private:
    // 返回底层数组的首地址
    char* begin();
    const char* begin() const;
    
    // 扩容或整理内存空间的内部函数
    void makeSpace(size_t len);
};