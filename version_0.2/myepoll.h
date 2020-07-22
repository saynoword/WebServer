#ifndef MY_EPOLL
#define MY_EPOLL

#define LISTENNUM 1024
#define MAX_EVENTS 5000

#include <sys/epoll.h>
#include <unordered_map>
#include <memory> 
#include <string>
#include <vector>
#include "requestData.h"
#include "threadpool.h"
#include <iostream>
#include "util.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

class myepoll{
private:
    static int epoll_fd;
    static std::unordered_map<int, std::shared_ptr<requestData>> fd2req;
    static struct epoll_event* events; 
    static const std::string PATH;
public:
    static int epoll_init(int maxevents, int listennum);
    static int epoll_add(int socket_fd, std::shared_ptr<requestData> request, __uint32_t events);
    static int epoll_mod(int socket_fd, std::shared_ptr<requestData> request, __uint32_t events);
    static int epoll_del(int socket_fd, __uint32_t events);
    static void my_epoll_wait(int listen_fd, int max_events, int timeout, std::shared_ptr<threadpool> pool);
    static void acceptConnection(int sock_fd, std::string path);
    static std::vector<std::shared_ptr<requestData>> getEvents(int sock_fd, int event_num, std::string path);
};

#endif
