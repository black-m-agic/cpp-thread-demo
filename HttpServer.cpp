#include "HttpServer.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>  // 用于 std::remove
#include <fstream>
#include <string>
// 获取HTTP状态码描述
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

// 构建HTTP响应报文（你的原版代码）
std::string build_http_response(const HttpResponse& response) {
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
  http_response += "\r\n";
  http_response += response.body;
  return http_response;
}

// 1. 获取文件MIME类型（告诉浏览器怎么解析文件）
std::string get_mime_type(const std::string& path) {
  if (path.find(".html") != std::string::npos) return "text/html";
  if (path.find(".css") != std::string::npos) return "text/css";
  if (path.find(".js") != std::string::npos) return "application/javascript";
  if (path.find(".jpg") != std::string::npos) return "image/jpeg";
  if (path.find(".png") != std::string::npos) return "image/png";
  if (path.find(".gif") != std::string::npos) return "image/gif";
  return "text/plain;charset=utf-8";
}

// 2. 读取本地文件内容
std::string read_file(const std::string& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) return "";
  return std::string((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
}

// 3. 解析HTTP请求（完整版）
HttpRequest parse_http_request(const std::string& request) {
  HttpRequest req;
  size_t method_end = request.find(" ");
  size_t path_end = request.find(" ", method_end + 1);

  req.method = request.substr(0, method_end);
  req.path = request.substr(method_end + 1, path_end - method_end - 1);
  req.version =
      request.substr(path_end + 1, request.find("\r\n") - path_end - 1);

  // 默认访问 / 时打开 index.html
  if (req.path == "/") req.path = "/index.html";
  return req;
}

// 4. 核心：处理HTTP请求 + 静态资源
void handle_http_request(int client_fd) {
  char buffer[1024] = {0};
  // ET模式下需要循环读取，直到请求头结束（\r\n\r\n）
  int total_len = 0;
  while (true) {
    int len =
        recv(client_fd, buffer + total_len, sizeof(buffer) - total_len, 0);
    if (len <= 0) return;
    total_len += len;
    if (std::string(buffer, total_len).find("\r\n\r\n") != std::string::npos)
      break;  // 请求头结束
  }
  // int len = recv(client_fd, buffer, sizeof(buffer), 0);
  if (total_len <= 0) return;
  HttpRequest req = parse_http_request(buffer);

  HttpResponse res;
  res.version = "HTTP/1.1";

  // 读取本地文件
  std::string file_content = read_file("." + req.path);
  if (!file_content.empty()) {
    res.status_code = 200;
    res.status_message = get_status_message(200);
    res.mime_type = get_mime_type(req.path);
    res.body = file_content;
  } else {
    // 文件不存在，返回404
    res.status_code = 404;
    res.status_message = get_status_message(404);
    res.mime_type = "text/html; charset=utf-8";
    res.body = "<h1>404 Not Found</h1>";
  }

  // 发送响应
  std::string response = build_http_response(res);
  send(client_fd, response.c_str(), response.size(), 0);
  close(client_fd);
}