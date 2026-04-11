#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd == -1) {
    perror("socket creation failed");
    return -1;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(8080);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind failed");
    return -2;
  }
  if (listen(listen_fd, 5) == -1) {
    perror("listen failed");
    return -3;
  }

  close(listen_fd);
  return 0;
}