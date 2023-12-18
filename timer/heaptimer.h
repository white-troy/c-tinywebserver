//使用最小堆来实现定时器的管理，堆顶为定时器唤醒间隔最低的值
//根据时间堆中超时时间最小的定时器来确定的
//即取出堆顶定时器的超时值作为心搏间隔时间。这个心搏间隔表示下一次定时器触发的时间间隔。
#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include<queue>
#include<unordered_map>
#include<time.h>
#include<algorithm>
#include<arpa/inet.h>
#include<functional>
#include<assert.h>
#include<chrono>
#include"../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimeNode{
    int id;
    TimeStamp expires;//超时时间
    TimeoutCallBack cb;//回调函数
    //重载操作符 >
    bool operator > (const TimeNode& t){
        //若t的超时时间大于超时时间，则返回true
        return expires < t.expires;
    }
    //重载操作符 <
    bool operator < (const TimeNode& t){
        //若t的超时时间小于超时时间，则返回true
        return expires > t.expires;
    }
};

class HeapTimer{
public:
    HeapTimer(){heap_.reserve(64);}//reseve是vector的方法，预先分配了内存来存储未来可能出现的元素
    ~HeapTimer() { clear();}

    void adjust(int id,int newExpires);
    void add(int id,int timeOut, const TimeoutCallBack& cb);
    void dowork(int id);
    void clear();
    void tick();
    void pop();
    int GetNextTick();

private:
    void del_(size_t i);
    void siftup_(size_t i);
    bool siftdown_(size_t i,size_t n);
    void SwapNode_(size_t i,size_t j);

    std::vector<TimeNode> heap_;//使用vector实现最小堆
    // key:id value:vector的下标；id对应的在heap_中的下标，方便用heap_的时候查找
    std::unordered_map<int,size_t>ref_;//使用哈希表实现对事件的查找
}



#endif