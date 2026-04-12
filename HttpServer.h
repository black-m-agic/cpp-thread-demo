#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <string>

// HTTP请求结构体（和你原代码完全一致）
struct HttpRequest {
  std::string method;
  std::string path;
  std::string version;
};

// HTTP响应结构体（和你原代码完全一致）
struct HttpResponse {
  std::string version;
  int status_code;
  std::string status_message;
  std::string mime_type;
  std::string body;
};

// 获取状态码描述信息
std::string get_status_message(int status_code);

// 构建HTTP响应报文
std::string build_http_response(const HttpResponse& response);

// 解析HTTP请求
HttpRequest parse_http_request(const std::string& request);

// 处理客户端HTTP请求（入口函数）
void handle_http_request(int client_fd);

#endif