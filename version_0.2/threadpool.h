#ifndef THREAD_POOL
#define THREAD_POOL

#define MAX_THREAD_NUM 16

#include <iostream>
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <memory>

class threadpool{
private:
    typedef std::function<void()> task;
    std::queue<task> tasks;
    std::vector<std::thread> pool;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> run;
    std::atomic<int> freeThrCnt;
public:
    threadpool(unsigned int size = 4); 
    ~threadpool();
    template<class F, class... Args>
    auto commit(F&& f, Args&&... args)
        ->std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task_ = std::make_shared<std::packaged_task<return_type()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> fut = task_->get_future();
        {
            std::unique_lock<std::mutex> lck(mtx);
            if(!run){
                throw std::runtime_error("ThreadPool is stopped");
            }
            tasks.emplace([task_]{(*task_)();});
            //log
            std::cout << "[ThreadPOOL]Commit success" << std::endl;
        }
#ifdef THREADPOOL_AUTO_GROW
        if(freeThrCnt < 0 && pool.size() < MAC_THREAD_NUM){
            addThread(1);
        }
#endif
        cv.notify_one();
        return fut;
    }
#ifdef THREADPOOL_AUTO_GROW
private:
#endif
    void addThread(unsigned int size);
public:
    int idlCount();
    int thrCount();
};
#endif
