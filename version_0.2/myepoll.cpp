#include "myepoll.h"

struct epoll_event* myepoll::events;
int myepoll::epoll_fd = 0;
std::unordered_map<int, std::shared_ptr<requestData>> myepoll::fd2req;
const std::string myepoll::PATH = "/";

int myepoll::epoll_init(int maxevents, int listennum){
    epoll_fd = epoll_create(listennum + 1);
    events = new epoll_event[maxevents];
    if(epoll_fd == -1){
        std::cerr << "Failed to create epoll_fd!" << std::endl;
        return -1;
    }
    return 0;
}

int myepoll::epoll_add(int socket_fd, std::shared_ptr<requestData> request, __uint32_t events){
    struct epoll_event ev;
    ev.data.fd = socket_fd;
    ev.events = events;
    fd2req[socket_fd] = request;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) == -1){
        std::cerr << "Failed to add event to epoll!" << std::endl; 
        return -1;
    }
    INFO("[EPOLL]:Epoll Add");
    return 0;
}

int myepoll::epoll_mod(int socket_fd, std::shared_ptr<requestData> request, __uint32_t events){
    struct epoll_event ev;
    ev.data.fd = socket_fd;
    ev.events = events;
    fd2req[socket_fd] = request;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, socket_fd, &ev) == -1){
        std::cerr << "Failed to mod epoll!" << std::endl;
        return -1; 
    }
    INFO("[EPOLL]:Epoll Mod");
    return 0;
}

int myepoll::epoll_del(int socket_fd, __uint32_t events){
    struct epoll_event ev;
    ev.data.fd = socket_fd;
    ev.events = events;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, socket_fd, &ev) == -1){
        std::cerr << "Failed to delete a event!" << std::endl;
        return -1; 
    }
    auto fd_it = fd2req.find(socket_fd);
    if(fd_it != fd2req.end()){
        fd2req.erase(fd_it);
    }
    INFO("[EPOLL]:Epoll Mod");
    return 0;
}

void myepoll::my_epoll_wait(int listen_fd, int max_events, int timeout, std::shared_ptr<threadpool> pool){
    INFO("[EPOLL]:Epoll Wait");
    int eventCnt = epoll_wait(epoll_fd, events, max_events, timeout);
    INFO("%s: %d", "[EPOLL]:Epoll wait counts", eventCnt);
    if(eventCnt < 0){
        std::cerr << "epoll wait error" << std::endl;
    }
    std::vector<std::shared_ptr<requestData>> req_data = getEvents(listen_fd, eventCnt, PATH);
    if(req_data.size() > 0){
        for(auto& req : req_data){
            // threadpoll options 
            if(req.use_count() > 0)
                std::future<void> fut = pool->commit(&requestData::handleRequest, req); 
        }
    }
}

std::vector<std::shared_ptr<requestData>> myepoll::getEvents(int listen_fd, int event_num, std::string path){
    std::vector<std::shared_ptr<requestData>> reqData;
    for(int i = 0; i < event_num; ++i){
        int fd = events[i].data.fd;
        if(fd == listen_fd){
            INFO("[EPOLL]:Epoll Get: listen_fd");
            myepoll::acceptConnection(listen_fd, path); 
        }
        else if(fd < 3){
            INFO("[EPOLL]Epoll Get: Invalid accept_fd!");
            break; 
        }
        else{
                // 排除错误事件
                if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                    || (!(events[i].events & EPOLLIN))){
                    auto fd_it = fd2req.find(fd);
                    if(fd_it != fd2req.end()){
                        fd2req.erase(fd_it);
                    }
                    continue;
                }
                INFO("%s%d","[EPOLL]:Epoll Get: write_fd:", fd);
                std::shared_ptr<requestData> curReq(fd2req[fd]);
                // if(curReq.use_count() == 0){
                //     continue; 
                // }
                std::cout << "curReq.use_count():" << curReq.use_count() << std::endl;
                curReq->seperateTimer();
                std::cout << "curReq.use_count() before push:" << curReq.use_count() << std::endl;
                reqData.push_back(curReq);
                std::cout << "curReq.use_count() after push:" << curReq.use_count() << std::endl;
                auto fd_it = fd2req.find(fd);
                if(fd_it != fd2req.end()){
                    fd2req.erase(fd_it);
                }
        }
    } 
    INFO("[EPOLL]:Epoll Get Finish");
    return reqData;
}

void myepoll::acceptConnection(int listen_fd, std::string path){
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(struct sockaddr_in));
    socklen_t client_addr_len = 0;
    int accept_fd = 0;
    while((accept_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len)) > 0){
        if(setSocketNonBlocking(accept_fd) == -1){
            std::cerr << "set nonblock error" << std::endl; 
            return;
        } 
        INFO("%s%d", "[EPOLL]Accept:accept_fd: ", accept_fd);
        std::shared_ptr<requestData> request = std::make_shared<requestData> (epoll_fd, accept_fd, path);
        __uint32_t _epo_event = EPOLLIN | EPOLLET | EPOLLONESHOT;
        epoll_add(accept_fd, request, _epo_event); 
        //bind mytimer and request
        std::shared_ptr<mytimer> timer = std::make_shared<mytimer>(request, TIMER_TIME_OUT);
        request->addTimer(timer);
        {
                std::unique_lock<std::mutex> lck(mytimer::timerMtx);
                mytimer::myTimerQueue.push(timer);
        }
    }
    INFO("[EPOLL]:Epoll Accept Finish");
}
