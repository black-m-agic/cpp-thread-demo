#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "HttpServer.h"
#include "TcpServer.h"
#include "ThreadPool.h"

#define MAX_EVENTS 2048
#define SUB_THREAD_COUNT 4  // 真正的多线程Reactor线程数

std::map<int, std::string> g_fd_to_req;
std::mutex g_map_mutex;

// 线程池：处理HTTP业务
ThreadPool pool(SUB_THREAD_COUNT);

// 子线程结构体：每个线程独立epoll
struct SubReactor {
  int epfd;
  std::thread thread;
  std::atomic<int> count{0};
};

std::vector<SubReactor> sub_reactors(SUB_THREAD_COUNT);

// 设置非阻塞
void set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// 子线程主循环：每个线程一个epoll，真正并行IO
void sub_reactor_loop(SubReactor* sub) {
  int epfd = sub->epfd;
  struct epoll_event events[MAX_EVENTS];

  while (true) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;

      char buf[4096];
      std::string req;
      bool need_close = false;

      while (true) {
        int len = recv(fd, buf, sizeof(buf), 0);
        if (len < 0) {
          if (errno == EAGAIN || errno == EWOULDBLOCK) break;
          need_close = true;
          break;
        } else if (len == 0) {
          need_close = true;
          break;
        }
        req.append(buf, len);
      }

      if (need_close) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
        continue;
      }

      if (req.find("\r\n\r\n") != std::string::npos) {
        {
          std::lock_guard<std::mutex> lock(g_map_mutex);
          g_fd_to_req[fd] = req;
        }
        pool.submit([fd]() { handle_http_request(fd); });
      }
    }
  }
}

// 主线程：只负责accept + 分配连接
int main() {
  int listen_fd = init_listen_socket();
  if (listen_fd < 0) return -1;

  int main_epfd = epoll_create1(0);
  struct epoll_event ev{};
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = listen_fd;
  epoll_ctl(main_epfd, EPOLL_CTL_ADD, listen_fd, &ev);

  // 启动所有子Reactor（每个子线程一个独立epoll）
  for (int i = 0; i < SUB_THREAD_COUNT; ++i) {
    sub_reactors[i].epfd = epoll_create1(0);
    sub_reactors[i].thread = std::thread(sub_reactor_loop, &sub_reactors[i]);
    sub_reactors[i].thread.detach();
  }

  std::cout << "【多线程Reactor】启动成功：主线程1个 + IO线程"
            << SUB_THREAD_COUNT << "个" << std::endl;

  // 轮询分配连接
  int next = 0;
  struct epoll_event events[MAX_EVENTS];
  while (true) {
    int n = epoll_wait(main_epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; ++i) {
      int fd = accept(listen_fd, nullptr, nullptr);
      if (fd < 0) continue;

      set_nonblock(fd);

      // 选择一个子Reactor
      SubReactor& sub = sub_reactors[next];
      next = (next + 1) % SUB_THREAD_COUNT;

      struct epoll_event add_ev{};
      add_ev.events = EPOLLIN | EPOLLET;
      add_ev.data.fd = fd;
      epoll_ctl(sub.epfd, EPOLL_CTL_ADD, fd, &add_ev);
    }
  }

  close(listen_fd);
  return 0;
}