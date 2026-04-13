#include "TcpServer.h"

#include <bits/socket.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>

// 初始化监听Socket
int init_listen_socket() {
  // 1. 创建socket
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::cerr << "创建 socket 失败" << std::endl;
    return -1;
  }

  // 2. 端口复用
  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // 3. 地址配置
  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(SERVER_PORT);

  // 4. 绑定
  if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    std::cerr << "绑定端口失败" << std::endl;
    close(listen_fd);
    return -1;
  }

  // 5. 监听
  if (listen(listen_fd, 1024) < 0) {  // 从 128 改成 1024
    std::cerr << "监听端口失败" << std::endl;
    close(listen_fd);
    return -1;
  }

  return listen_fd;
}

// 初始化Epoll
int init_epoll(int listen_fd) {
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    std::cerr << "创建 epoll 实例失败" << std::endl;
    return -1;
  }

  struct epoll_event ev{};
  ev.events = EPOLLIN | EPOLLET;  // 边缘触发
  ev.data.fd = listen_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

  return epoll_fd;
}