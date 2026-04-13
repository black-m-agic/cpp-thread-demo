#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>  // 新增：锁
#include <string>

#include "HttpServer.h"
#include "TcpServer.h"
#include "ThreadPool.h"

#define MAX_EVENTS 2048

// 全局变量 + 互斥锁（修复并发崩溃）
std::map<int, std::string> g_fd_to_req;
std::mutex g_map_mutex;  // 核心：保护map

int main() {
  const int THREAD_NUM = 4;

  int listen_fd = init_listen_socket();
  if (listen_fd < 0) return -1;

  int epoll_fd = init_epoll(listen_fd);
  if (epoll_fd < 0) {
    close(listen_fd);
    return -1;
  }

  ThreadPool pool(THREAD_NUM);
  std::cout << "epoll 服务器已启动，高并发监听端口 8080" << std::endl;

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
        if (conn_fd < 0) continue;

        int flags = fcntl(conn_fd, F_GETFL, 0);
        fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK);

        struct epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev);
      } else {
        int conn_fd = curr_fd;
        char temp_buf[4096] = {0};
        std::string full_req;
        bool need_close = false;

        // ET模式：一次性读完数据
        while (true) {
          int len = recv(conn_fd, temp_buf, sizeof(temp_buf), 0);
          if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            need_close = true;
            break;
          } else if (len == 0) {
            need_close = true;
            break;
          }
          full_req.append(temp_buf, len);
        }

        if (need_close) {
          // 安全关闭：先移除epoll，再关fd，最后清map
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn_fd, nullptr);
          close(conn_fd);
          std::lock_guard<std::mutex> lock(g_map_mutex);
          g_fd_to_req.erase(conn_fd);
          continue;
        }

        // 只处理完整HTTP请求
        if (full_req.find("\r\n\r\n") != std::string::npos) {
          // 加锁存数据
          std::lock_guard<std::mutex> lock(g_map_mutex);
          g_fd_to_req[conn_fd] = full_req;

          // 提交任务
          auto task = [conn_fd]() { handle_http_request(conn_fd); };
          pool.submit(task);
        }

        // 长连接核心：保留FD，不重复MOD事件（修复无限触发）
      }
    }
  }

  close(listen_fd);
  close(epoll_fd);
  return 0;
}