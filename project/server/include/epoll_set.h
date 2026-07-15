#ifndef EPOLL_SET_H
#define EPOLL_SET_H

#define PORT 8888
#define MAX_EVENTS 1024
#define BUFFER_SIZE 1024

// 客户端会话结构体
typedef struct
{
    int fd;      // 文件描述符
    char id[64]; // 设备ID或管理员账号
    int type;    // 0:未认证, 1:设备, 2:管理员
    char buffer[BUFFER_SIZE];
    int buf_len;
} ClientSession;

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

void epoll_set(void);
int set_nonblocking(int fd);

extern ClientSession sessions[MAX_EVENTS];

#endif
