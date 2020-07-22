#ifndef UTIL
#define UTIL
#include <cstdlib>

ssize_t readn(int fd, void *buff, size_t n);
ssize_t writen(int fd, void *buff, size_t n);
void handle_for_sigpipe();
int setSocketNonBlocking(int fd);

#define INFO(format, ...) printf(format "\n", ##__VA_ARGS__)
#endif
