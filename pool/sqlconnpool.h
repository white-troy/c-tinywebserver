//数据库连接池
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include<mysql/mysql.h>
#include<string>
#include<queue>
#include<mutex>
#include<semaphore.h>
#include<thread>
#include"../log/log.h"

class SqlConnPool{
public:
    //静态实例
    static SqlConnPool* Instance();

    MYSQL* GetConn();
    void FreeConn(MYSQL* conn);
    int GetFreeConnCount();
    
    void Init(const char* host,uint16_t port,
              const char* user,const char* pwd,
              const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool() = default;
    ~SqlConnPool() {
        ClosePool();
    }

    int MAX_CONN_;
    
    std::queue<MYSQL*> connQue_;
    std::mutex mtx_;
    sem_t semId_;
};

class SqlConnRAII{
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool* connpool){
        assert(connpool);
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }

    ~SqlConnRAII(){
        if(sql_){
            connpool_->FreeConn(sql_);
        }
    }

private:
    MYSQL* sql_;
    SqlConnPool* connpool_;
};
#endif