#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#define MAX_EVENTS 1024

struct HttpRequest {
  std::string method;
  std::string path;
  std::string version;
};

struct HttpResponse {
  std::string version;
  int status_code;
  std::string status_message;
  std::string mime_type;
  std::string body;
};

std::string get_status_message(int status_code) {
  switch (status_code) {
    case 200:
      return "OK";
    case 400:
      return "Bad Request";
    case 404:
      return "Not Found";
    case 500:
      return "Internal Server Error";
    default:
      return "Unknown Status";
  }
}

std::string build_http_response(const HttpResponse& response) {
  // 1. 彻底清除version里的所有换行符，双重保险
  std::string clean_version = response.version;
  clean_version.erase(
      std::remove(clean_version.begin(), clean_version.end(), '\r'),
      clean_version.end());
  clean_version.erase(
      std::remove(clean_version.begin(), clean_version.end(), '\n'),
      clean_version.end());

  std::string http_response;
  http_response += clean_version + " " + std::to_string(response.status_code) +
                   " " + response.status_message + "\r\n";
  http_response += "Content-Type: " + response.mime_type + "\r\n";
  http_response +=
      "Content-Length: " + std::to_string(response.body.size()) + "\r\n";
  http_response += "\r\n";  // 空行分隔头部和正文
  http_response += response.body;
  return http_response;
}

int main() {
  // 1. 创建监听socket
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::cerr << "创建 socket 失败" << std::endl;
    return -1;
  }
  // 2. 配置地址，端口复用
  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8080);
  // 3. 绑定端口
  if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) <
      0) {
    std::cerr << "绑定端口失败" << std::endl;
    close(listen_fd);
    return -1;
  }
  // 4. 开始监听
  if (listen(listen_fd, 128) < 0) {
    std::cerr << "监听端口失败" << std::endl;
    close(listen_fd);
    return -1;
  }
  std::cout << "epoll 服务器已启动，高并发监听端口 8080" << std::endl;
  // 5. 创建 epoll 实例并注册监听 socket
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    std::cerr << "创建 epoll 实例失败" << std::endl;
    close(listen_fd);
    return -1;
  }
  // 6. 将监听 socket 注册到 epoll 实例中，关注可读事件
  struct epoll_event ev{}, events[MAX_EVENTS];
  ev.events = EPOLLIN;
  ev.data.fd = listen_fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);
  // 7. 事件循环，等待事件发生并处理
  while (true) {
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds < 0) {
      std::cerr << "epoll_wait 失败" << std::endl;
      break;
    }

    for (int i = 0; i < nfds; i++) {
      int curr_fd = events[i].data.fd;
      if (curr_fd == listen_fd) {
        // 处理新连接
        int conn_fd = accept(listen_fd, nullptr, nullptr);
        if (conn_fd < 0) {
          std::cerr << "接受连接失败" << std::endl;
          continue;
        }
        std::cout << "新连接已接受，文件描述符: " << conn_fd << std::endl;
        ev.events = EPOLLIN;
        ev.data.fd = conn_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev);
      }
      // 处理现有连接的数据
      else {
        char buffer[1024] = {0};
        int len = recv(curr_fd, buffer, sizeof(buffer) - 1, 0);
        // 连接关闭或发生错误
        if (len <= 0) {
          std::cout << "客户端断开，文件描述符: " << curr_fd << std::endl;
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, curr_fd, nullptr);
          close(curr_fd);
          continue;
        } else {
          // 处理现有连接的数据
          // 输出接收到的数据
          std::cout << "接收到数据: " << buffer
                    << " 来自文件描述符: " << curr_fd << std::endl;
          // const char* response = "服务端已接收";
          // HTTP 协议有铁律：服务端返回给浏览器的第一行数据，必须是 HTTP
          // 响应行（HTTP/1.1 200 OK） send(curr_fd, response, strlen(response),
          // 0);  // 回显确认消息 send(curr_fd, buffer, len, 0); // 回显数据

          // 解析 HTTP 请求（简单示例，未处理完整 HTTP 协议）
          std::string request(buffer);
          size_t method_end = request.find('\r\n');
          // 兼容异常情况，用\n兜底
          if (method_end == std::string::npos) {
            method_end = request.find('\n');
          }

          std::string method_line = request.substr(0, method_end);
          // 2. 彻底清理请求行里的\r\n，确保解析干净
          method_line.erase(
              std::remove(method_line.begin(), method_line.end(), '\r'),
              method_line.end());
          method_line.erase(
              std::remove(method_line.begin(), method_line.end(), '\n'),
              method_line.end());
          std::cout << "接收到 HTTP 请求: " << method_line << std::endl;
          // 3. 拆分method、path、version
          HttpRequest http_request;
          size_t space1 = method_line.find(' ');
          if (space1 == std::string::npos) {
            std::cerr << "无效的 HTTP 请求行" << std::endl;
            close(curr_fd);
            continue;
          }

          http_request.method = method_line.substr(0, space1);
          size_t space2 = method_line.find(' ', space1 + 1);
          if (space2 == std::string::npos) {
            std::cerr << "无效的 HTTP 请求行" << std::endl;
            close(curr_fd);
            continue;
          }

          http_request.path =
              method_line.substr(space1 + 1, space2 - space1 - 1);
          http_request.version = method_line.substr(space2 + 1);

          // 输出解析结果
          std::cout << "HTTP 方法: " << http_request.method << std::endl;
          std::cout << "请求路径: " << http_request.path << std::endl;
          std::cout << "HTTP 版本: " << http_request.version << std::endl;
          // std::string response_body =
          //     http_request.version + " 200 OK\r\n\r\nHello HTTP Server";
          // send(curr_fd, response_body.c_str(), response_body.size(), 0);

          // 构建 HTTP 响应
          HttpResponse http_response;
          http_response.version = http_request.version;
          http_response.mime_type = "text/html; charset=utf-8";
          // 根据请求路径设置响应内容
          if (http_request.path == "/") {
            http_response.status_code = 200;
            http_response.status_message =
                get_status_message(http_response.status_code);
            http_response.body =
                "<html><head><meta "
                "charset=\"UTF-8\"></head><body><h1>欢迎访问首页！</h1></"
                "body></html>";
          } else if (http_request.path == "/hello") {
            http_response.status_code = 200;
            http_response.status_message =
                get_status_message(http_response.status_code);
            http_response.body =
                "<html><head><meta charset=\"UTF-8\"></head><body><h1>Hello, "
                "HTTP Server!</h1></body></html>";
          } else {
            http_response.status_code = 404;
            http_response.status_message =
                get_status_message(http_response.status_code);
            http_response.body =
                "<html><head><meta "
                "charset=\"UTF-8\"></head><body><h1>页面未找到</h1></body></"
                "html>";
          }
          // 构建完整 HTTP 响应字符串并发送
          std::string response_str = build_http_response(http_response);
          send(curr_fd, response_str.c_str(), response_str.size(), 0);
          // 处理完请求后关闭连接
          close(curr_fd);
        }
      }
    }
  }
  // 8. 清理资源
  close(listen_fd);
  close(epoll_fd);
  return 0;
}