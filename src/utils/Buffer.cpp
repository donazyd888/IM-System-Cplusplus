#include "utils/Buffer.h"
#include <sys/uio.h>  // 用于 readv
#include <unistd.h>
#include <cstring>

Buffer::Buffer(size_t initialSize)
    : buffer_(initialSize), readPos_(0), writePos_(0) {
}

size_t Buffer::readableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::writableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::prependableBytes() const {
    return readPos_;
}

const char* Buffer::peek() const {
    return begin() + readPos_;
}

void Buffer::retrieve(size_t len) {
    if (len < readableBytes()) {
        readPos_ += len;
    } else {
        retrieveAll();
    }
}

void Buffer::retrieveUntil(const char* end) {
    retrieve(end - peek());
}

void Buffer::retrieveAll() {
    readPos_ = 0;
    writePos_ = 0;
}

std::string Buffer::retrieveAllAsString() {
    return retrieveAsString(readableBytes());
}

std::string Buffer::retrieveAsString(size_t len) {
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

void Buffer::ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
        makeSpace(len);
    }
}

void Buffer::append(const char* data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, begin() + writePos_);
    writePos_ += len;
}

void Buffer::append(const std::string& str) {
    append(str.data(), str.length());
}

// 核心系统调用实现
ssize_t Buffer::readFd(int fd, int* savedErrno) {
    // 申请一段 64KB 的栈上临时空间
    char extrabuf[65536];
    
    // 使用 iovec 结构体实现分散读 (scatter read)
    struct iovec vec[2];
    const size_t writable = writableBytes();
    
    // 第一块缓冲区：我们自己的 std::vector 中的可写空间
    vec[0].iov_base = begin() + writePos_;
    vec[0].iov_len = writable;
    
    // 第二块缓冲区：栈上的 64KB 临时空间
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // 如果我们自带的缓冲区足够大，就只用一块；否则用两块
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    
    // readv 是一次系统调用，它可以将数据直接读入多个分离的缓冲区中
    const ssize_t n = ::readv(fd, vec, iovcnt);
    
    if (n < 0) {
        *savedErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        // 数据全读进第一块（我们自己的 vector）中了，只需移动写指针
        writePos_ += n;
    } else {
        // 第一块满了，超出的部分读到了 extrabuf 中
        writePos_ = buffer_.size(); // 自己的 vector 写满了
        // 将 extrabuf 中的数据再追加到我们的 Buffer 中（此时会触发 vector 扩容）
        append(extrabuf, n - writable);
    }
    return n;
}

char* Buffer::begin() {
    return &*buffer_.begin();
}

const char* Buffer::begin() const {
    return &*buffer_.begin();
}

void Buffer::makeSpace(size_t len) {
    // 如果“可写空间”+“前面闲置的空间”加起来都不够 len
    if (writableBytes() + prependableBytes() < len) {
        // 直接扩容底层的 vector
        buffer_.resize(writePos_ + len);
    } else {
        // 空间其实是够的，只是数据靠后了。我们将现有数据挪到最前面
        size_t readable = readableBytes();
        std::copy(begin() + readPos_, begin() + writePos_, begin());
        readPos_ = 0;
        writePos_ = readPos_ + readable;
    }
}