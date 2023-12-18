#include"epoller.h"

//构造函数
Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)),events_(maxEvent){
    assert(epollFd_ >= 0 && events_.size()>0);
}

//析构函数
Epoller::~Epoller(){
    close(epollFd_);
}

bool Epoller::AddFd(int fd, uint32_t events){
    if(fd < 0) return fasle;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&ev);
}

bool Epoller::ModFd(int fd,uint32_t events){
    if(fd < 0) return fasle;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd_,EPOLL_CTL_MOD,fd,&ev);
}

bool Epoller::DelFd(int fd,uint32_t events){
    if(fd < 0) return fasle;
    return 0 == epoll_ctl(epollFd_,EPOLL_CTL_DEL,fd,0);
}

//返回事件数量
int Epoller::Wait(int timeoutMS){
    return epoll_wait(epollFd_,&events_[0],static_cast<int>(events_.size()),timeoutMS);
}

//获取事件fd
int Epoller::GetEventFd(size_t i) const{
    assert(i< events_.size()&& i>=0);
    return events_[i].data.fd;
}

//获取事件属性
uint32_t Epoller::GetEvents(size_t i) const{
    assert(a<events_.size()&&i>=0);
    return events_[i].events;
}