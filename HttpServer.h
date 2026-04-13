#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <map>
#include <mutex>
#include <string>

// 前置声明
struct SubReactor;

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

std::string get_status_message(int status_code);
std::string build_http_response(const HttpResponse& response);
HttpRequest parse_http_request(const std::string& request);
// 🔥 无锁改造：直接传入请求数据，无全局共享
void handle_http_request(int client_fd, const std::string& full_req);
std::string get_mime_type(const std::string& path);
std::string read_file(const std::string& path);

#endif