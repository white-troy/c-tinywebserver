#ifndef BUFFER_H
#define BUFFER_H

#include<cstring>
#include<iostream>
#include<unistd.h>
//#include<io.h>
//linux下定义改为uio.h
#include<sys/uio.h>
#include<vector>
#include<atomic>
#include<assert.h>
class Buffer{
public:
    //构造函数
    Buffer(int initBufferSize = 1024);
    //析构函数
    ~Buffer()=default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    const char* Peek() const;
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);

    void RetrieveAll();
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buffer);

    ssize_t ReadFd(int fd, int* Errno);
    ssize_t WriteFd(int fd, int* Errno);

private:
    char* BeginPtr_();//buffer头部
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char> buffer_;
    std::atomic<std::size_t> readPos_;//读的下标
    std::atomic<std::size_t> writePos_;//写的下标

};

#endif