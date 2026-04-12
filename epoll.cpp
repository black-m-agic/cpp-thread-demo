#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#define MAX_EVENTS 1024

struct HttpRequest {
  std::string method;
  std::string path;
  std::string version;
};

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8080);
  bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  listen(listen_fd, 128);
  std::cout << "epoll 服务器已启动，高并发监听端口 8080" << std::endl;
  int epoll_fd = epoll_create1(0);
  struct epoll_event ev{}, events[MAX_EVENTS];
  ev.events = EPOLLIN;
  ev.data.fd = listen_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
  while (true) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for (int i = 0; i < nfds; i++) {
      int curr_fd = events[i].data.fd;
      if (curr_fd == listen_fd) {
        // 处理新连接
        int conn_fd = accept(listen_fd, nullptr, nullptr);
        std::cout << "新连接已接受，文件描述符: " << conn_fd << std::endl;
        ev.events = EPOLLIN;
        ev.data.fd = conn_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev);
      } else {
        char buffer[1024] = {0};
        int len = recv(curr_fd, buffer, sizeof(buffer), 0);
        if (len <= 0) {
          // 连接关闭或发生错误
          std::cout << "客户端断开，文件描述符: " << curr_fd << std::endl;
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, nullptr);
          close(curr_fd);
          continue;
        } else {
          // 处理现有连接的数据
          // 输出接收到的数据
          std::cout << "接收到数据: " << buffer
                    << " 来自文件描述符: " << curr_fd << std::endl;
          const char* response = "服务端已接收";
          send(curr_fd, response, strlen(response), 0);  // 回显确认消息
          send(curr_fd, buffer, len, 0);                 // 回显数据
          // 解析 HTTP 请求（简单示例，未处理完整 HTTP 协议）
          std::string request(buffer);
          size_t method_end = request.find('\n');
          std::string method_line = request.substr(0, method_end);
          std::cout << "接收到 HTTP 请求: " << method_line << std::endl;
          HttpRequest http_request;
          // 查找空格分隔符，提取 HTTP 方法、路径和版本
          size_t space1 = method_line.find(' ');
          http_request.method = method_line.substr(0, space1);
          size_t space2 = method_line.find(' ', space1 + 1);
          http_request.path =
              method_line.substr(space1 + 1, space2 - space1 - 1);
          http_request.version = method_line.substr(space2 + 1);
          // 输出解析结果
          std::cout << "HTTP 方法: " << http_request.method << std::endl;
          std::cout << "请求路径: " << http_request.path << std::endl;
          std::cout << "HTTP 版本: " << http_request.version << std::endl;
          std::string response_body =
              http_request.version + " 200 OK\r\n\r\nHello HTTP Server";
          send(curr_fd, response_body.c_str(), response_body.size(), 0);
        }
      }
    }
  }
  close(listen_fd);
  close(epoll_fd);
  return 0;
}