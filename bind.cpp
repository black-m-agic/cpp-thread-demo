#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>

// int bind(int sockfd, const struct sockaddr *addr, socketlen_t addrlen);
// sockfd: 由socket()函数返回的套接字描述符。
// addr: 指向一个sockaddr结构的指针，包含了要绑定的地址
// 和端口信息。这个结构通常是sockaddr_in（用于IPv4）或sockaddr_in6（用于IPv6）的一个实例。
// addrlen: addr结构的长度，通常使用sizeof(struct sockaddr_in)或sizeof(struct
// sockaddr_in6)。

int main() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket creation failed");
    return -1;
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(8080);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  int ret = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

  if (ret == -1) {
    perror("bind failed");
    return -2;
  }
  return 0;
}