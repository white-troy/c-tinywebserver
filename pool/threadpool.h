//线程池
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<thread>
#include<assert.h>

class ThreadPool{
public:
    //默认构造函数，将对象的成员变量处于默认初始化
    ThreadPool() = default;
    //将一个右值引用（ThreadPool&&）传给默认构造函数
    ThreadPool(ThreadPool&&) = default;
    //用make_shared代替new，将对象的创建和智能指针的初始化合并在一起
    //如果通过new再传递给shared_ptr，内存是不连续的，会造成内存碎片化
    //使用explicit显示的声明构造函数和转换函数
    explicit ThreadPool(int threadCount = 8):pool_(std::make_shared<Pool>()){
        // make_shared:传递右值
        //功能是在动态内存中分配一个对象并初始化它，返回指向此对象的shared_ptr
        assert(threadCount > 0);
        for(int i =0;i<threadCount;i++){
            //创建一个新线程，并执行lambda表达式的代码块，使用[this]捕获当前对象的引用
            std::thread([this](){
                std::unique_lock<std::mutex> locker(pool_->mtx_);
                while(true){
                    //判断线程池中的任务队列是否为空，非空则表示有任务
                    if(!pool_->tasks.empty()){
                        //取出任务队列中的第一个任务，并使用move变为右值
                        //目的是为了将当前任务转移给当前线程，防止多线程争夺同一个任务
                        auto task = std::move(pool_->tasks.front());
                        pool_->tasks.pop();//弹出刚刚取出的任务
                        locker.unlock();//已经取出任务，解锁，方便task的执行
                        task();//执行刚刚队列中的任务
                        locker.lock();//上锁，循环等待下一个任务
                    }
                    //判断线程池是否关闭
                    else if(pool_->isClosed){
                        break;//若关闭则跳出循环
                    }
                    else{
                        //调用条件变量的wait方法，将当前线程置于等待状态
                        //直到有新任务被添加到队列中或者线程池关闭
                        //在此期间互斥锁被释放，方便其他线程上锁添加新任务
                        pool_->cond_.wait(locker);
                    }
                }
            }).detach();
        }
    }

    //析构函数
    ~ThreadPool(){
        if(pool_){
            std::unique_lock<std::mutex> locker(pool_->mtx_);
            pool_->isClosed = true;
        }
        pool_->cond_.notify_all();//唤醒所有线程
    }

    template<typename T>
    void AddTask(T&& task){
        std::unique_lock<std::mutex> locker(pool_->mtx_);
        pool_->tasks.emplace(std::forward<T>(task));
        pool_->cond_.notify_one();
    }

private:
    //使用结构体封装
    struct Pool{
        std::mutex mtx_;
        std::condition_variable cond_;
        bool isClosed;
        std::queue<std::function<void()>> tasks;//任务队列
    };
    //智能指针
    std::shared_ptr<Pool> pool_;
    
};

#endif