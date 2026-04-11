#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    perror("socket creation failed");
    return -1;
  }

  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("bind failed");
    return -2;
  }

  if (listen(listen_fd, 5) == -1) {
    perror("listen failed");
    return -3;
  }
  std::cout << "✅ 服务端启动成功,正在监听8080端口,等待客户端连接..."
            << std::endl;
  struct sockaddr_in client_addr{};
  socklen_t client_addr_len = sizeof(client_addr);
  int conn_fd =
      accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
  if (conn_fd == -1) {
    perror("accept failed");
    return -4;
  }
  std::cout << "Client connected! socket fd: " << conn_fd << std::endl;

  close(conn_fd);
  close(listen_fd);
  return 0;
}