#include "HttpServer.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>  // 新增
#include <string>

extern std::map<int, std::string> g_fd_to_req;
extern std::mutex g_map_mutex;  // 引入锁

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
  http_response += "Connection: keep-alive\r\n";
  http_response += "\r\n";
  http_response += response.body;
  return http_response;
}

std::string get_mime_type(const std::string& path) {
  if (path.find(".html") != std::string::npos) return "text/html";
  if (path.find(".css") != std::string::npos) return "text/css";
  if (path.find(".js") != std::string::npos) return "application/javascript";
  if (path.find(".jpg") != std::string::npos) return "image/jpeg";
  if (path.find(".png") != std::string::npos) return "image/png";
  if (path.find(".gif") != std::string::npos) return "image/gif";
  return "text/plain;charset=utf-8";
}

std::string g_index_cache;
std::string read_file(const std::string& path) {
  if (path == "./index.html" || path == "/index.html" || path == "index.html") {
    if (g_index_cache.empty()) {
      FILE* fp = fopen(path.c_str(), "rb");
      if (fp) {
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        g_index_cache.resize(size);
        size_t ret = fread(&g_index_cache[0], 1, size, fp);
        (void)ret;
        fclose(fp);
      }
    }
    return g_index_cache;
  }

  std::string content;
  FILE* fp = fopen(path.c_str(), "rb");
  if (fp) {
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    content.resize(size);
    size_t ret = fread(&content[0], 1, size, fp);
    (void)ret;
    fclose(fp);
  }
  return content;
}

HttpRequest parse_http_request(const std::string& request) {
  HttpRequest req;
  size_t method_end = request.find(" ");
  size_t path_end = request.find(" ", method_end + 1);

  req.method = request.substr(0, method_end);
  req.path = request.substr(method_end + 1, path_end - method_end - 1);
  req.version =
      request.substr(path_end + 1, request.find("\r\n") - path_end - 1);

  if (req.path == "/") req.path = "/index.html";
  return req;
}

void handle_http_request(int client_fd) {
  std::string full_req;
  // 加锁读/删数据（修复并发崩溃）
  {
    std::lock_guard<std::mutex> lock(g_map_mutex);
    auto it = g_fd_to_req.find(client_fd);
    if (it == g_fd_to_req.end()) return;
    full_req = it->second;
    g_fd_to_req.erase(it);
  }

  HttpRequest req = parse_http_request(full_req);
  HttpResponse res;
  res.version = "HTTP/1.1";

  std::string file_content = read_file("." + req.path);
  if (!file_content.empty()) {
    res.status_code = 200;
    res.status_message = get_status_message(200);
    res.mime_type = get_mime_type(req.path);
    res.body = file_content;
  } else {
    res.status_code = 404;
    res.status_message = get_status_message(404);
    res.mime_type = "text/html; charset=utf-8";
    res.body = "<h1>404 Not Found</h1>";
  }

  std::string response = build_http_response(res);
  // 安全发送：忽略错误（长连接正常断开）
  send(client_fd, response.c_str(), response.size(),
       MSG_DONTWAIT | MSG_NOSIGNAL);
  // send(client_fd, response.c_str(), response.size(), MSG_NOSIGNAL);
}