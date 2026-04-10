#include <stdio.h>
#include <sys/socket.h>
// int socket(int domain, int type, int protocol);
// AF_INET: IPv4 Internet protocols
// AF_INET6: IPv6 Internet protocols
// AF_UNIX: Local communication
// SOCK_STREAM: Provides sequenced, reliable, two-way, connection-based byte
// streams. SOCK_DGRAM: Supports datagrams (connectionless, unreliable messages
// of a fixed maximum length). SOCK_RAW: Provides raw network protocol access.
// SOCK_SEQPACKET: Provides a sequenced, reliable, two-way connection-based
// datagram service. protocol: 0表示使用默认协议，通常与type参数相关联。
// 例如，对于SOCK_STREAM类型，默认协议是TCP；对于SOCK_DGRAM类型，默认协议是UDP。

int main() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd == -1) {
    perror("socket creation failed");
    return -1;
  }

  int sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd2 == -1) {
    perror("socket creation failed");
    return -2;
  }

  int sockfd3 = socket(AF_UNIX, SOCK_STREAM, 0);

  return 0;
}