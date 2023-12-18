#include "log.h"

//构造函数
Log::Log(){
    fp_ = nullptr;//打开log的文件指针
    deque_ = nullptr;//阻塞队列
    writeThread_ = nullptr;//写线程的指针
    lineCount_ = 0;//日志行数记录
    toDay_ = 0;//按当天日期区分文件
    isAsync_ = false;//是否开启异步日志
}

//析构函数
Log::~Log(){
    //当阻塞队列非空时，唤醒消费者处理剩余任务，直至清空队列
    while(!deque_->empty()){
        deque_->flush();//唤醒消费者
    }
    deque_->Close();//关闭队列
    writeThread_->join();//等待当前线程完成手中任务
    if(fp_){ //清洗文件缓冲区，关闭文件描述符
        lock_guard<mutex> locker(mtx_);
        flush();//清空缓冲区数据
        fclose(fp_);//关闭日志文件
    }
}

//唤醒阻塞队列消费者，开始写日志
void Log::flush(){
    if(isAsync_){//只有异步日志用deque
        deque_->flush();
    }
    fflush(fp_);//清空输入缓存区
}

//单例模式的懒汉模式（用到才唤醒），局部静态变量法（无需加锁解锁）
Log* Log::Instance(){
    static Log log;
    return &log;//返回对象的地址
}

//异步日志的写线程函数
void Log::FlushLogThread(){
    Log::Instance()->AsncWrite_();
}

//写线程真正的执行函数
void Log::AsncWrite_(){
    string str = "";
    while (deque_->pop(str)){
        lock_guard<mutex> locker(mtx_);
        fputs(str.c_str(),fp_);
    }   
}

//初始化日志实例
void Log::init(int level,const char* path, const char* suffix, int maxQueCapacity){
    isOpen_ = true;
    level_ =level;
    path_ = path;
    suffix_ = suffix;
    if(maxQueCapacity){//异步方式
        isAsync_ = true;
        if(!deque_){ //如果没有deque，则new一个
            unique_ptr<BlockQueue<std::string>> newQue(new BlockQueue<std::string>);
            //因为unique_ptr不支持普通的拷贝或赋值操作,所以采用move
            //将动态申请的内存权给deque，newDeque被释放
            deque_ = move(newQue);  // 左值变右值,掏空newDeque

            unique_ptr<thread> newThread(new thread(FlushLogThread));
            writeThread_ = move(newThread);
        }
    }
    else{
        isAsync_ = false;
    }

    lineCount_ = 0;
    time_t timer = time(nullptr);
    struct tm* sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName,LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
            path_,t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,suffix_);
    toDay_ = t.tm_mday; 
    //锁的范围
    {
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();
        if(fp_){
            flush();
            fclose(fp_);
        }
        fp_ = fopen(fileName,"a");//打开文件读取并附加写入
        if(fp_ == nullptr){
            mkdir(path_,0777);//生成目录文件（最大权限）
            fp_ = fopen(fileName,"a")
        }
        assert(fp_!=nullptr);
    }
}

void Log::write(int level, const char* format,...){
    struct timeval now{0,0};
    gettimeofday(&now,nullptr);
    time_t tSec = now.tv_sec;
    struct tm* sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    //日志日期 日志行数 如果不是今天或者行数超了
    if(toDay_!=t.tm_mday||(lineCount_&&(lineCount_ % MAX_LINES_ == 0))){
        unique_lock<mutex> locker(mtx_);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        //如果时间不匹配，则替换为最新的日志文件名
        if(toDay_ != t.tm_mday){
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
            toDay_ = t.tm_mday;
            lineCount_ = 0;
        }
        else{
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (lineCount_  / MAX_LINES), suffix_);
        }

        locker.lock();
        flush();
        fclose(fp_);
        fp_ = fopen(newFile,"a");
        assert(fp_!=nullptr;)
    }

    //锁的内容，在buffer中生成一条对应的日志信息
    {
        unique_lock<mutex> locker(mtx_);
        lineCount_ ++;
        int n = snprintf(buff_.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        buff_.HasWritten(n);
        AppendLogLevelTitle_(level);

        va_start(va_list,format);//stdarg.h
        int m = vsnprintf(buff_.BeginWrite(), buff_.WritableBytes(), format, vaList);
        va_end(vaList);

        buff_.HasWritten(m);
        buff_.Append("\n\0",2);
        //如果阻塞队列大小不为0则使用异步写入，等于0则使用同步写入
        if(isAsync_ && deque_ && !deque_->full()) { // 异步方式（加入阻塞队列中，等待写线程读取日志信息）
            deque_->push_back(buff_.RetrieveAllToStr());
        } else {    // 同步方式（直接向文件中写入日志信息）
            fputs(buff_.Peek(), fp_);   // 同步就直接写入文件
        }
        buff_.RetrieveAll();    // 清空buff
    }
}

//添加日志等级
void Log::AppendLogLevelTitle_(int level){
    switch (level)
    {
    case 0://调试代码时的输出
        buff_.Append("[debug]: ",9);
        break;
    case 1://系统当前状态
        buff_.Append("[info]: ",9);
        break;
    case 2://调试代码的警告
        buff_.Append("[warn]: ",9);
        break;
    case 3://系统的错误信息
        buff_.Append("[error]: ",9);
        break;
    default:
        buff_.Append("[info]: ", 9)
        break;
    }
}

int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}