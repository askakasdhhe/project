#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define PORT 8888

extern int server_fd;

int socket_set(void);
int set_nonblocking(int fd);

#endif
