#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include "myepoll.h"
#include "requestData.h"
#include "util.h"
#include <string.h>
#include <memory>

#define PORT 8888
#define MAX_EVENTS 5000
#define LISTENNUM 1024


int socket_bind_listen(int port);
void handle_expired_event();

int main(){
    INFO("[SERVER]:Server Start!");
    if(myepoll::epoll_init(MAX_EVENTS, LISTENNUM) == -1){
        std::cerr << "epoll init failed" << std::endl;
        return 1;
    }
    std::shared_ptr<threadpool> pool = std::make_shared<threadpool>(4);
    int listen_fd = socket_bind_listen(PORT);
    if(listen_fd < 0){
        std::cerr << "socket bind listen failed" << std::endl;  
        return 1;
    }
    if(setSocketNonBlocking(listen_fd) == -1){
        std::cerr << "setNonBlocking failed" << std::endl; 
        return 1;
    }
    __uint32_t events = EPOLLIN | EPOLLET;
    std::shared_ptr<requestData> request(new requestData());
    request->setFd(listen_fd);
    if(myepoll::epoll_add(listen_fd, request, events) == -1){
        std::cerr << "epoll add error" << std::endl; 
        return 1;
    }
    int epolladd = myepoll::epoll_add(listen_fd, NULL, events);
    std::cout << "epolladd:" << epolladd << std::endl;
    INFO("[SERVER]:Configuration complete!");
    while(true){
        myepoll::my_epoll_wait(listen_fd, MAX_EVENTS, -1, pool); 
        mytimer::handle_expired();
    }
    return 0; 
}

int socket_bind_listen(int port){
    if(port < 1024 || port > 65535){
        std::cerr << "wrong port: " << port << std::endl;
        return -1;
    }
    int socket_fd;
    int flag = false;
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd == -1){
        std::cerr << "socket failed!" << std::endl;
        return -1;
    }
    int optval = 1;
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1){
        std::cerr << "set socket failed!" << std::endl;
        return -1;
    }
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if(bind(socket_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        std::cerr << "bind failed" << std::endl; 
        return -1;
    }
    if(listen(socket_fd, LISTENNUM) == -1){
        std::cerr << "listen failed!" << std::endl; 
        return -1;
    }
    return socket_fd;
}

void handle_expired(){
    
}
