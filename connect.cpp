#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

int main() {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd == -1) {
    perror("socket creation failed");
    return -1;
  }
  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("connect failed");
    return -2;
  }
  std::cout << "✅ 客户端连接成功! 127.0.0.1:8080" << std::endl;
  close(client_fd);
  std::cout << "客户端已关闭连接" << std::endl;
  return 0;
}