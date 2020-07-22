#include "threadpool.h"

threadpool::threadpool(unsigned int size):run(true), freeThrCnt(0){
    addThread(size);
}

threadpool::~threadpool(){
    run = false;
    cv.notify_all();
    for(std::thread& thr : pool){
        if(thr.joinable()){
            thr.join(); 
        }
    }
}


void threadpool::addThread(unsigned int size){
    for(; pool.size() < MAX_THREAD_NUM && size > 0; --size){
        pool.emplace_back([this]{
            while(run){
                task task_;
                {
                    std::unique_lock<std::mutex> lck(mtx); 
                    cv.wait(lck, [this]{return !run || !tasks.empty();});
                    if(!run && tasks.empty()){
                        return;
                    }
                    task_ = std::move(tasks.front());
                    tasks.pop();
                }
                --freeThrCnt;
                task_();
                ++freeThrCnt; 
            }
        });
        ++freeThrCnt;
    }
}
int threadpool::idlCount(){
    return freeThrCnt; 
}
int threadpool::thrCount(){
    return pool.size(); 
}
