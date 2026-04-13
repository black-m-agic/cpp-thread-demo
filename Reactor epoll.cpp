#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "HttpServer.h"
#include "TcpServer.h"
#include "ThreadPool.h"

#define MAX_EVENTS 2048
#define SUB_THREAD_COUNT 4

// 无锁！无全局map！无全局锁！
ThreadPool pool(8);

struct SubReactor {
  int epfd;
  std::thread thread;
};

std::vector<SubReactor> sub_reactors(SUB_THREAD_COUNT);

void set_nonblock(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void sub_reactor_loop(SubReactor* sub) {
  int epfd = sub->epfd;
  struct epoll_event events[MAX_EVENTS];

  while (true) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; ++i) {
      int fd = events[i].data.fd;

      char buf[8192];
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

      // 🔥 真正无锁：数据直接移动到lambda，零拷贝、无共享、无锁
      if (req.find("\r\n\r\n") != std::string::npos) {
        pool.submit([fd, request = std::move(req)]() {
          handle_http_request(fd, request);
        });
      }
    }
  }
}

int main() {
  int opt = 1;
  setsockopt(0, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  int listen_fd = init_listen_socket();
  if (listen_fd < 0) return -1;

  int main_epfd = epoll_create1(0);
  struct epoll_event ev{};
  ev.events = EPOLLIN | EPOLLET;
  ev.data.fd = listen_fd;
  epoll_ctl(main_epfd, EPOLL_CTL_ADD, listen_fd, &ev);

  for (int i = 0; i < SUB_THREAD_COUNT; ++i) {
    sub_reactors[i].epfd = epoll_create1(0);
    sub_reactors[i].thread = std::thread(sub_reactor_loop, &sub_reactors[i]);
    sub_reactors[i].thread.detach();
  }

  std::cout << "【无锁多线程Reactor】启动成功 | 无全局锁/无全局map"
            << std::endl;

  int next = 0;
  struct epoll_event events[MAX_EVENTS];
  while (true) {
    int n = epoll_wait(main_epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; ++i) {
      int client_fd = accept(listen_fd, nullptr, nullptr);
      if (client_fd < 0) continue;

      set_nonblock(client_fd);

      SubReactor& sub = sub_reactors[next];
      next = (next + 1) % SUB_THREAD_COUNT;

      struct epoll_event add_ev{};
      add_ev.events = EPOLLIN | EPOLLET;
      add_ev.data.fd = client_fd;
      epoll_ctl(sub.epfd, EPOLL_CTL_ADD, client_fd, &add_ev);
    }
  }

  close(listen_fd);
  return 0;
}