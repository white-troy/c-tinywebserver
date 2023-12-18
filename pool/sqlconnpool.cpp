/*
 * @Author: K2SO4 zs1115996793@qq.com
 * @Date: 2023-12-11 19:39:31
 * @LastEditors: K2SO4 zs1115996793@qq.com
 * @LastEditTime: 2023-12-11 19:52:21
 * @FilePath: \c-tinywebserver\pool\sqlconnpool.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "sqlconnpool.h"

SqlConnPool* SqlConnPool::Instance(){
    static SqlConnPool pool;
    return &pool;
}

//初始化
void SqlConnPool::Init(const char* host, uint16_t port,
                       const char* user, const char* pwd,
                       const cahr* dbName, int connSize = 10){
    assert(connSize>0);
    for(int i=0;i<connSize;i++){
        MYSQL* conn = nullptr;
        conn = mysql_init(conn);
        if(!conn){
            LOG_ERROR("mysql init error");
            assert(conn);
        }
        conn = mysql_real_connect(conn,host,user,pwd,dbName,port,nullptr,0);
        if(!conn){
            LOG_ERROR("mysql connect error");
        }
        connQue_.emplace(conn);
    }
    MAX_CONN_=connSize;
    sem_init(&semId_,0,MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn(){
    MYSQL* conn = nullptr;
    if(connQue_.empty()){
        LOG_WARN("SqlConnPool busy");
        return nullptr;
    }
    sem_wait(&semId_);//-1
    lock_guard<mutex> locker(mtx_);
    conn = connQue_.front();
    connQue_.pop();
    return conn;
}

void SqlConnPool::FreeConn(MYSQL* conn){
    assert(conn);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(conn);
    sem_post(&semId_);//+1
}

void SqlConnPool::ClosePool(){
    lock_guard<mutex> locker(mtx_);
    while(!connQue_.empty()){
        auto conn = connQue_.front();
        connQue_.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount(){
    lock_guard<mutex> locker(mtx_);
    return connQue_.size();
}