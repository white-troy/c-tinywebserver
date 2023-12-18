#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include<deque>
#include<condition_variable>
#include<mutex>
#include<sys/time.h>

using namespace std;

//使用template保证了复用性，可以用不同类型(int or str)来实例化类
template<typename T>
class BlockQueue{
public:
    //explicit防止意外的隐式类型转换，代表该构造函数是一个显示构造函数
    explicit BlockQueue(size_t maxsize=1000);
    ~BlockQueue();
    bool empty();
    bool full();

    void push_back(const T& item);
    void push_front(const T& item);
    bool pop(T& item); //弹出的任务放入item
    bool pop(T& item ,int timeout);//等待时间
    void clear();

    T front();
    T back();
    size_t capacity();
    size_t size();

    void flush();
    void Close();

private:
    deque<T> deq_;//双端队列来充当底层的数据结构
    mutex mtx_;//锁
    bool isClose_;//关闭标志
    size_t capacity_;//容量
    
    condition_variable condConsumer_;//消费者条件变量
    condition_variable condProducer_;//生产者条件变量
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t maxsize):capacity_(maxsize){
    assert(maxsize>0);//实例化时传入的容量要大于0
    isClose_=false;
}

template<typename T>
BlockQueue<T>::~BlockQueue(){
    Close();
}

template<typename T>//关闭阻塞队列
void BlockQueue<T>::Close(){
    clear();//清空队列中所有元素
    isClose_=true;//设置关闭标志
    condConsumer_.notify_all();//唤醒所有消费者线程，通过isclose状态使线程退出
    condProducer_.notify_all();//唤醒所有生产者线程，通过isclose状态使线程退出
}

template<typename T>
void BlockQueue<T>::clear(){
    //在多线程并发中，通过锁来保证当前线程获取的变量不会被竞争
    lock_guard<mutex> locker(mtx_);
    deq_.clear();//在互斥锁的条件下清空阻塞队列中所有元素，保证在清空时队列不会被其他线程访问
}

template<typename T>
bool BlockQueue<T>::empty() {
    lock_guard<mutex> locker(mtx_);
    return deq_.empty();
}

template<typename T>//判断队列是否满
bool BlockQueue<T>::full() {
    lock_guard<mutex> locker(mtx_);
    //判断当前队列是否已经达到或者超过容量限制，即返回队列是否已满
    return deq_.size() >= capacity_;
}

/*
deq_中，生产者线程负责向队列中添加元素，消费者线程负责取走元素
生产者通过push_front/push_back往deq_中添加item，当满的时候，暂停
消费者通过pop取走deq_的item,当空的时候，暂停
*/
template<typename T>
void BlockQueue<T>::push_back(const T& item){
    //注意，条件变量需要搭配unique_lock
    unique_lock<mutex> locker(mtx_);
    while(deq_.size() >= capacity_){//队列满了，等待
        //暂停生产，等待消费者唤醒生产条件变量
        //即等待notify_one()的唤醒
        condProducer_.wait(locker); 
    }
    deq_.push_back(item);
    condConsumer_.notify_one();//唤醒消费者消费
}

template<typename T>
void BlockQueue<T>::push_front(const T& item) {
    unique_lock<mutex> locker(mtx_);
    while(deq_.size() >= capacity_) {//队列满了，等待
        condProducer_.wait(locker);  //暂停生产，等待消费者唤醒生产条件变量
    }
    deq_.push_front(item);
    condConsumer_.notify_one();//唤醒消费者消费
}

template<typename T>
bool BlockQueue<T>::pop(T& item){
    unique_lock<mutex> locker(mtx_);
    while(deq_.empty()){
        condConsumer_.wait(locker);//队列空，等待notify_one()的唤醒
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();//唤醒生产者生产
    return true;
}

template<typename T>//添加等待时间的变量
bool BlockQueue<T>::pop(T& item,int timeout){
    unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        if(condConsumer_.wait_for(locker,std::chrono::seconds(timeout))==std::cv_status::timeout){
            return false;
        }
        if(isClose_){
            return false;
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<typename T>
T BlockQueue<T>::front(){
    lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<typename T>
T BlockQueue<T>::back() {
    lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template<typename T>
size_t BlockQueue<T>::capacity() {
    lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<typename T>
size_t BlockQueue<T>::size() {
    lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

//唤醒一个消费者
template<typename T>
void BlockQueue<T>::flush(){
    //唤醒消费者线程
    condConsumer_.notify_one();
}

#endif
