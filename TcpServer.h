#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <iostream>

// 配置
#define MAX_EVENTS 2048
#define SERVER_PORT 8080

// 初始化监听Socket (创建+复用+绑定+监听)
int init_listen_socket();

// 初始化Epoll (创建实例+添加监听fd)
int init_epoll(int listen_fd);

#endif