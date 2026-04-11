#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

int main() {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  std::cout << "连接成功！发送消息：hello epoll\n";
  send(client_fd, "hello epoll", 10, 0);

  char buf[1024] = {0};
  recv(client_fd, buf, 1024, 0);
  std::cout << "收到回复：" << buf << std::endl;

  close(client_fd);
  return 0;
}