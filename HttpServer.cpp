#include "HttpServer.h"

#include <algorithm>  // 用于 std::remove
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