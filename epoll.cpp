#include <fcntl.h>  // 非阻塞函数必需
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

#include "HttpServer.h"
#include "TcpServer.h"
#include "ThreadPool.h"  //使用线程池处理请求
#define MAX_EVENTS 1024

int main() {
  const int THREAD_NUM = 4;  // 线程数配置

  // 1. 调用封装好的初始化函数
  int listen_fd = init_listen_socket();
  if (listen_fd < 0) return -1;

  int epoll_fd = init_epoll(listen_fd);
  if (epoll_fd < 0) {
    close(listen_fd);
    return -1;
  }

  // 2. 线程池
  ThreadPool pool(THREAD_NUM);
  std::cout << "epoll 服务器已启动，高并发监听端口 8080" << std::endl;

  // 3. 核心事件循环
  struct epoll_event events[MAX_EVENTS];
  while (true) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds < 0) {
      std::cerr << "epoll_wait 失败" << std::endl;
      break;
    }

    for (int i = 0; i < nfds; i++) {
      int curr_fd = events[i].data.fd;
      if (curr_fd == listen_fd) {
        int conn_fd = accept(listen_fd, nullptr, nullptr);
        // 设置非阻塞
        //

        if (conn_fd < 0) continue;
        std::cout << "新连接已接受，文件描述符: " << conn_fd << std::endl;

        int flags = fcntl(conn_fd, F_GETFL, 0);
        fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK);

        struct epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;  // 边缘触发
        ev.data.fd = conn_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev);
      } else {
        int conn_fd = curr_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_fd, nullptr);

        pool.submit([conn_fd]() { handle_http_request(conn_fd); });
      }
    }
  }

  // 4. 资源清理
  close(listen_fd);
  close(epoll_fd);
  return 0;
}